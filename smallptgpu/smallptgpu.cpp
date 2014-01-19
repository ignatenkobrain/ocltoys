/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of OCLToys.                                         *
 *                                                                         *
 *   OCLToys is free software; you can redistribute it and/or modify       *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   OCLToys is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 *   OCLToys website: http://code.google.com/p/ocltoys                     *
 ***************************************************************************/

#if defined(WIN32)
// Required for M_PI
#define _USE_MATH_DEFINES
#endif

#include "ocltoy.h"
#include "camera.h"
#include "geom.h"

#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>

#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/thread.hpp>

#define GAMMA_TABLE_SIZE 1024

class SmallPTGPU : public OCLToy {
public:
	SmallPTGPU() : OCLToy("SmallPTGPU v" OCLTOYS_VERSION_MAJOR "." OCLTOYS_VERSION_MINOR " (OCLToys: http://code.google.com/p/ocltoys)") {
		millisTimerFunc = 100;
		kernelToneMapping = NULL;
		mergedPixels = NULL;

		currentSphere = 0;
		maxDepth = 6;
		defaultVolumeSigmaS = 0.f;
		defaultVolumeSigmaA = 0.f;

		pixelsBuff = NULL;
		
		const float gamma = 2.2f;
		float x = 0.f;
		const float dx = 1.f / GAMMA_TABLE_SIZE;
		for (unsigned int i = 0; i < GAMMA_TABLE_SIZE; ++i, x += dx)
			gammaTable[i] = powf(std::max(std::min(x, 1.f), 0.f), 1.f / gamma);
	}

	virtual ~SmallPTGPU() {
		FreeBuffers();
		
		for (unsigned int i = 0; i < selectedDevices.size(); ++i)
			delete kernelsSmallPT[i];
		delete kernelToneMapping;
	}

protected:
	boost::program_options::options_description GetOptionsDescriction() {
		boost::program_options::options_description opts("SmallPTGPU options");

		opts.add_options()
			("kernel,k", boost::program_options::value<std::string>()->default_value("preprocessed_rendering_kernel.cl"),
				"OpenCL kernel file name")
			("scene,n", boost::program_options::value<std::string>()->default_value("scenes/cornell.scn"),
				"Filename of the scene to render")
			("workgroupsize,z", boost::program_options::value<size_t>(), "OpenCL workgroup size");

		return opts;
	}

	virtual int RunToy() {
		samplesBuff.resize(selectedDevices.size(), NULL);
		seedsBuff.resize(selectedDevices.size(), NULL);
		cameraBuff.resize(selectedDevices.size(), NULL);
		spheresBuff.resize(selectedDevices.size(), NULL);

		pixels.resize(selectedDevices.size(), NULL);
		sampleSec.resize(selectedDevices.size(), 0.0);
		currentSample.resize(selectedDevices.size(), 0);

		ReadScene(commandLineOpts["scene"].as<std::string>());

		SetUpOpenCL();
		StartRendering();

		glutMainLoop();

		return EXIT_SUCCESS;
	}

	//--------------------------------------------------------------------------
	// GLUT related code
	//--------------------------------------------------------------------------

	virtual void DisplayCallBack() {
		glRasterPos2i(0, 0);
		if (selectedDevices.size() == 1)
			glDrawPixels(windowWidth, windowHeight, GL_RGB, GL_FLOAT, pixels[0]);
		else {
			// Multiple devices, I have to merge the results and to apply tone mapping
			const unsigned count = windowWidth * windowHeight * 3;
			std::copy(pixels[0], pixels[0] + count, mergedPixels);

			for (unsigned int i = 1; i < selectedDevices.size(); ++i) {
				for (unsigned int j = 0; j < count; ++j)
					mergedPixels[j] += pixels[i][j];
			}

			const float scale = 1.f / selectedDevices.size();
			for (unsigned int i = 0; i < count; ++i)
				mergedPixels[i] = Radiance2PixelFloat(scale * mergedPixels[i]);

			glDrawPixels(windowWidth, windowHeight, GL_RGB, GL_FLOAT, mergedPixels);
		}

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(0.f, 0.f, 0.f, 0.8f);
		glRecti(0, windowHeight - 15,
				windowWidth - 1, windowHeight - 1);
		glRecti(0, 0, windowWidth - 1, 18);
		glDisable(GL_BLEND);

		// Title
		glColor3f(1.f, 1.f, 1.f);
		glRasterPos2i(4, windowHeight - 10);
		PrintString(GLUT_BITMAP_8_BY_13, windowTitle.c_str());

		// Caption line 0
		double globalSampleSec = 0.0;
		unsigned int globalPass = 0;
		for (unsigned int i = 0; i < selectedDevices.size(); ++i) {
			globalSampleSec += sampleSec[i];
			globalPass += currentSample[i] + 1;
		}
		
		const std::string captionString = boost::str(boost::format("[Pass %d][%.1fM Sample/sec]") %
				globalPass % (globalSampleSec / 1000000.0));

		glColor3f(1.f, 1.f, 1.f);
		glRasterPos2i(4, 5);
		PrintString(GLUT_BITMAP_8_BY_13, captionString.c_str());

		if (printHelp) {
			glPushMatrix();
			glLoadIdentity();
			glOrtho(-.5f, windowWidth - .5f,
				-.5f, windowHeight - .5f, -1.f, 1.f);

			PrintHelp();

			glPopMatrix();
		}

		glutSwapBuffers();
	}

	//--------------------------------------------------------------------------
	// GLUT call backs
	//--------------------------------------------------------------------------

	virtual void ReshapeCallBack(int newWidth, int newHeight) {
		StopRendering();

		windowWidth = newWidth;
		windowHeight = newHeight;

		glViewport(0, 0, windowWidth, windowHeight);
		glLoadIdentity();
		glOrtho(-.5f, windowWidth - .5f,
				-.5f, windowHeight - .5f, -1.f, 1.f);

		UpdateCamera();
		UpdateCameraBuffer();
		ResizeFrameBuffer();
		UpdateKernelsArgs();

		StartRendering();

		glutPostRedisplay();
	}

#define MOVE_STEP 0.5f
#define ROTATE_STEP (2.f * ((float)M_PI) / 180.f)
	virtual void KeyCallBack(unsigned char key, int x, int y) {
		bool cameraUpdated = false;
		bool sceneUpdated = false;
		bool needRedisplay = true;

		switch (key) {
			case 'p': {
				// Write image to PPM file
				std::ofstream f("image.ppm", std::ofstream::trunc);
				if (!f.good()) {
					OCLTOY_LOG("Failed to open image file: image.ppm");
				} else {
					f << "P3" << std::endl;
					f << windowWidth << " " << windowHeight << std::endl;
					f << "255" << std::endl;

					const float *img = (selectedDevices.size() == 1) ? pixels[0] : mergedPixels;
					for (int y = (int)windowHeight - 1; y >= 0; --y) {
						const float *p = &img[y * windowWidth * 3];
						for (int x = 0; x < (int)windowWidth; ++x) {
							const float rv = std::min(std::max(*p++, 0.f), 1.f);
							const std::string r = boost::lexical_cast<std::string>((int)(rv * 255.f + .5f));
							const float gv = std::min(std::max(*p++, 0.f), 1.f);
							const std::string g = boost::lexical_cast<std::string>((int)(gv * 255.f + .5f));
							const float bv = std::min(std::max(*p++, 0.f), 1.f);
							const std::string b = boost::lexical_cast<std::string>((int)(bv * 255.f + .5f));
							f << r << " " << g << " " << b << std::endl;
						}
					}
				}
				f.close();
				OCLTOY_LOG("Saved framebuffer in image.ppm");

				needRedisplay = false;
				break;
			}
			case 27: // Escape key
			case 'q':
			case 'Q':
				StopRendering();
				OCLTOY_LOG("Done");

				exit(EXIT_SUCCESS);
				break;
			case ' ': // Restart rendering
				StopRendering();
				StartRendering();
				break;
			case 'h':
				printHelp = (!printHelp);
				break;
			case 'a': {
				Vec dir = camera.x;
				vnorm(dir);
				vsmul(dir, -MOVE_STEP, dir);
				vadd(camera.orig, camera.orig, dir);
				vadd(camera.target, camera.target, dir);
				cameraUpdated = true;
				break;
			}
			case 'd': {
				Vec dir = camera.x;
				vnorm(dir);
				vsmul(dir, MOVE_STEP, dir);
				vadd(camera.orig, camera.orig, dir);
				vadd(camera.target, camera.target, dir);
				cameraUpdated = true;
				break;
			}
			case 'w': {
				Vec dir = camera.dir;
				vsmul(dir, MOVE_STEP, dir);
				vadd(camera.orig, camera.orig, dir);
				vadd(camera.target, camera.target, dir);
				cameraUpdated = true;
				break;
			}
			case 's': {
				Vec dir = camera.dir;
				vsmul(dir, -MOVE_STEP, dir);
				vadd(camera.orig, camera.orig, dir);
				vadd(camera.target, camera.target, dir);
				cameraUpdated = true;
				break;
			}
			case 'r': {
				camera.orig.y += MOVE_STEP;
				camera.target.y += MOVE_STEP;
				cameraUpdated = true;
				break;
			}
			case 'f': {
				camera.orig.y -= MOVE_STEP;
				camera.target.y -= MOVE_STEP;
				cameraUpdated = true;
				break;
			}
			case '+':
				currentSphere = (currentSphere + 1) % spheres.size();
				OCLTOY_LOG("Selected sphere " << currentSphere << " (" <<
						spheres[currentSphere].p.x << " " <<
						spheres[currentSphere].p.y << " " <<
						spheres[currentSphere].p.z << ")");
				sceneUpdated = true;
				break;
			case '-':
				currentSphere = (currentSphere + (spheres.size() - 1)) % (unsigned int)spheres.size();
				OCLTOY_LOG("Selected sphere " << currentSphere << " (" <<
						spheres[currentSphere].p.x << " " <<
						spheres[currentSphere].p.y << " " <<
						spheres[currentSphere].p.z << ")");
				sceneUpdated = true;
				break;
			case '4':
				spheres[currentSphere].p.x -= 0.5f * MOVE_STEP;
				sceneUpdated = true;
				break;
			case '6':
				spheres[currentSphere].p.x += 0.5f * MOVE_STEP;
				sceneUpdated = true;
				break;
			case '8':
				spheres[currentSphere].p.z -= 0.5f * MOVE_STEP;
				sceneUpdated = true;
				break;
			case '2':
				spheres[currentSphere].p.z += 0.5f * MOVE_STEP;
				sceneUpdated = true;
				break;
			case '9':
				spheres[currentSphere].p.y += 0.5f * MOVE_STEP;
				sceneUpdated = true;
				break;
			case '3':
				spheres[currentSphere].p.y -= 0.5f * MOVE_STEP;
				sceneUpdated = true;
				break;
			default:
				needRedisplay = false;
				break;
		}

		if (cameraUpdated || sceneUpdated) {
			StopRendering();

			if (cameraUpdated) {
				UpdateCamera();
				UpdateCameraBuffer();
			}

			if (sceneUpdated)
				UpdateSpheresBuffer();

			StartRendering();
		}

		if (needRedisplay)
			glutPostRedisplay();
	}

	void SpecialCallBack(int key, int x, int y) {
		bool cameraUpdated = false;
		bool needRedisplay = true;

        switch (key) {
			case GLUT_KEY_UP: {
				Vec t = camera.target;
				vsub(t, t, camera.orig);
				t.y = t.y * cosf(-ROTATE_STEP) + t.z * sinf(-ROTATE_STEP);
				t.z = -t.y * sinf(-ROTATE_STEP) + t.z * cosf(-ROTATE_STEP);
				vadd(t, t, camera.orig);
				camera.target = t;
				cameraUpdated = true;
				break;
			}
			case GLUT_KEY_DOWN: {
				Vec t = camera.target;
				vsub(t, t, camera.orig);
				t.y = t.y * cosf(ROTATE_STEP) + t.z * sinf(ROTATE_STEP);
				t.z = -t.y * sinf(ROTATE_STEP) + t.z * cosf(ROTATE_STEP);
				vadd(t, t, camera.orig);
				camera.target = t;
				cameraUpdated = true;
				break;
			}
			case GLUT_KEY_LEFT: {
				Vec t = camera.target;
				vsub(t, t, camera.orig);
				t.x = t.x * cosf(-ROTATE_STEP) - t.z * sinf(-ROTATE_STEP);
				t.z = t.x * sinf(-ROTATE_STEP) + t.z * cosf(-ROTATE_STEP);
				vadd(t, t, camera.orig);
				camera.target = t;
				cameraUpdated = true;
				break;
			}
			case GLUT_KEY_RIGHT: {
				Vec t = camera.target;
				vsub(t, t, camera.orig);
				t.x = t.x * cosf(ROTATE_STEP) - t.z * sinf(ROTATE_STEP);
				t.z = t.x * sinf(ROTATE_STEP) + t.z * cosf(ROTATE_STEP);
				vadd(t, t, camera.orig);
				camera.target = t;
				cameraUpdated = true;
				break;
			}
			case GLUT_KEY_PAGE_UP:
				camera.target.y += MOVE_STEP;
				cameraUpdated = true;
				break;
			case GLUT_KEY_PAGE_DOWN:
				camera.target.y -= MOVE_STEP;
				cameraUpdated = true;
				break;
			default:
				needRedisplay = false;
				break;
		}

		if (cameraUpdated) {
			StopRendering();

			UpdateCamera();
			UpdateCameraBuffer();

			StartRendering();
		}

		if (needRedisplay)
			glutPostRedisplay();
	}

	void TimerCallBack(int id) {
		glutPostRedisplay();

		// Refresh the timer
		glutTimerFunc(millisTimerFunc, &OCLToy::GlutTimerFunc, 0);
	}

	//--------------------------------------------------------------------------
	// OpenCL related code
	//--------------------------------------------------------------------------

	virtual unsigned int GetMaxDeviceCountSupported() const {
		return std::numeric_limits<unsigned int>::max();
	}

private:
	void SetUpOpenCL() {
		//----------------------------------------------------------------------
		// Compile kernel
		//----------------------------------------------------------------------

		const std::string &kernelFileName = commandLineOpts["kernel"].as<std::string>();
		OCLTOY_LOG("Compile OpenCL kernel: " << kernelFileName);

		// Read the kernel
		const std::string kernelSource = ReadSources(kernelFileName);

		// Kernel options
		std::stringstream ss;
		ss.precision(6);
		ss << std::scientific <<
				"-DPARAM_MAX_DEPTH=" << maxDepth << " "
				"-DPARAM_DEFAULT_SIGMA_S=" << defaultVolumeSigmaS << "f "
				"-DPARAM_DEFAULT_SIGMA_A=" << defaultVolumeSigmaA << "f "
				"-I. -I../common";
		const std::string opts = ss.str();
		OCLTOY_LOG("Kernel parameters: " << opts);

		// Compile the kernel for each device
		kernelsSmallPT.resize(selectedDevices.size(), NULL);
		kernelsWorkGroupSize.resize(selectedDevices.size(), 0);
		cl::Program::Sources source(1, std::make_pair(kernelSource.c_str(), kernelSource.length()));
		for (unsigned int i = 0; i < selectedDevices.size(); ++i) {
			// Create the kernel program
			cl::Device &oclDevice = selectedDevices[i];
			cl::Context &oclContext = deviceContexts[i];
			cl::Program program = cl::Program(oclContext, source);
			try {
				VECTOR_CLASS<cl::Device> buildDevice;
				buildDevice.push_back(oclDevice);
				program.build(buildDevice, opts.c_str());
			} catch (cl::Error err) {
				cl::STRING_CLASS strError = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(oclDevice);
				OCLTOY_LOG("Kernel compilation error:\n" << strError.c_str());

				throw err;
			}

			kernelsSmallPT[i] = new cl::Kernel(program, "SmallPTGPU");
			kernelsSmallPT[i]->getWorkGroupInfo<size_t>(oclDevice, CL_KERNEL_WORK_GROUP_SIZE, &kernelsWorkGroupSize[i]);
			if (commandLineOpts.count("workgroupsize"))
				kernelsWorkGroupSize[i] = commandLineOpts["workgroupsize"].as<size_t>();
			OCLTOY_LOG("Using workgroup size (Device " + boost::lexical_cast<std::string>(i) + "): " << kernelsWorkGroupSize[i]);

			if ((selectedDevices.size() == 1) && (i == 0))
				kernelToneMapping = new cl::Kernel(program, "ToneMapping");
		}
		
		//----------------------------------------------------------------------
		// Allocate buffer
		//----------------------------------------------------------------------

		AllocateBuffers();

		//----------------------------------------------------------------------
		// Set kernel arguments
		//----------------------------------------------------------------------

		UpdateKernelsArgs();

		UpdateCameraBuffer();
		UpdateSpheresBuffer();
	}

	void FreeBuffers() {
		for (unsigned int i = 0; i < selectedDevices.size(); ++i) {
			FreeOCLBuffer(0, &samplesBuff[i]);			
			delete[] pixels[i];
			pixels[i] = NULL;
			FreeOCLBuffer(0, &seedsBuff[i]);
			FreeOCLBuffer(0, &cameraBuff[i]);
			FreeOCLBuffer(0, &spheresBuff[i]);
		}

		FreeOCLBuffer(0, &pixelsBuff);

		delete[] mergedPixels;
		mergedPixels = NULL;
	}

	void AllocateBuffers() {
		for (unsigned int i = 0; i < selectedDevices.size(); ++i) {
			AllocOCLBufferRO(i, &cameraBuff[i], &camera, sizeof(Camera),
					"CameraBuffer (Device " + boost::lexical_cast<std::string>(i) + ")");
			AllocOCLBufferRO(i, &spheresBuff[i], &spheres[0], sizeof(Sphere) * spheres.size(),
					"SpheresBuffer (Device " + boost::lexical_cast<std::string>(i) + ")");
		}

		// Allocate the frame buffer
		ResizeFrameBuffer();
	}

	void ReadScene(const std::string &fileName) {
		OCLTOY_LOG("Reading scene: " << fileName);

		std::ifstream f(fileName.c_str(), std::ifstream::in | std::ifstream::binary);
		if (!f.good())
			throw std::runtime_error("Failed to open file: " + fileName);

		// Read the camera position
		std::string cameraLine;
		std::getline(f, cameraLine);
		if (!f.good())
			throw std::runtime_error("Failed to read camera parameters");

		std::vector<std::string> cameraArgs;
		boost::split(cameraArgs, cameraLine, boost::is_any_of("\t "));
		if (cameraArgs.size() != 7)
			throw std::runtime_error("Failed to parse 6 camera parameters");

		camera.orig.x = boost::lexical_cast<float>(cameraArgs[1]);
		camera.orig.y = boost::lexical_cast<float>(cameraArgs[2]);
		camera.orig.z = boost::lexical_cast<float>(cameraArgs[3]);
		camera.target.x = boost::lexical_cast<float>(cameraArgs[4]);
		camera.target.y = boost::lexical_cast<float>(cameraArgs[5]);
		camera.target.z = boost::lexical_cast<float>(cameraArgs[6]);
		UpdateCamera();

		// Read the max. path depth
		std::string maxDepthLine;
		std::getline(f, maxDepthLine);
		if (!f.good())
			throw std::runtime_error("Failed to read path max. depth parameter");

		std::vector<std::string> maxDepthArgs;
		boost::split(maxDepthArgs, maxDepthLine, boost::is_any_of("\t "));
		if (maxDepthArgs.size() != 2)
			throw std::runtime_error("Failed to parse path max. depth parameter");

		maxDepth = boost::lexical_cast<unsigned int>(maxDepthArgs[1]);

		// Read the default sigma s value
		std::string sigmaSLine;
		std::getline(f, sigmaSLine);
		if (!f.good())
			throw std::runtime_error("Failed to read sigma s parameter");

		std::vector<std::string> sigmaSArgs;
		boost::split(sigmaSArgs, sigmaSLine, boost::is_any_of("\t "));
		if (sigmaSArgs.size() != 2)
			throw std::runtime_error("Failed to parse sigma s parameter");

		defaultVolumeSigmaS = boost::lexical_cast<float>(sigmaSArgs[1]);

		// Read the default sigma s value
		std::string sigmaALine;
		std::getline(f, sigmaALine);
		if (!f.good())
			throw std::runtime_error("Failed to read sigma s parameter");

		std::vector<std::string> sigmaAArgs;
		boost::split(sigmaAArgs, sigmaALine, boost::is_any_of("\t "));
		if (sigmaAArgs.size() != 2)
			throw std::runtime_error("Failed to parse sigma s parameter");

		defaultVolumeSigmaA = boost::lexical_cast<float>(sigmaAArgs[1]);

		// Read the sphere count
		std::string sizeLine;
		std::getline(f, sizeLine);
		if (!f.good())
			throw std::runtime_error("Failed to read sphere count");

		std::vector<std::string> sizeArgs;
		boost::split(sizeArgs, sizeLine, boost::is_any_of("\t "));
		if (sizeArgs.size() != 2)
			throw std::runtime_error("Failed to parse sphere count");
		
		const unsigned int sphereCount = boost::lexical_cast<unsigned int>(sizeArgs[1]);
		spheres.resize(sphereCount);

		// Read all spheres
		for (unsigned int i = 0; i < sphereCount; i++) {
			OCLTOY_LOG("  Parsing sphere " << i << "...");

			// Read the sphere definition
			std::string sphereLine;
			std::getline(f, sphereLine);
			if (!f.good())
				throw std::runtime_error("Failed to read sphere #" + boost::lexical_cast<std::string>(i));

			std::vector<std::string> sphereArgs;
			boost::split(sphereArgs, sphereLine, boost::is_any_of("\t "));
			if ((sphereArgs.size() < 8))
				throw std::runtime_error("Failed to parse initial parameters of sphere #" + boost::lexical_cast<std::string>(i));

			Sphere *s = &spheres[i];
			s->rad = boost::lexical_cast<float>(sphereArgs[1]);
			s->p.x = boost::lexical_cast<float>(sphereArgs[2]);
			s->p.y = boost::lexical_cast<float>(sphereArgs[3]);
			s->p.z = boost::lexical_cast<float>(sphereArgs[4]);
			s->e.x = boost::lexical_cast<float>(sphereArgs[5]);
			s->e.y = boost::lexical_cast<float>(sphereArgs[6]);
			s->e.z = boost::lexical_cast<float>(sphereArgs[7]);

			const unsigned int material = boost::lexical_cast<int>(sphereArgs[8]);
			switch (material) {
				case 0: {
					if ((sphereArgs.size() != 12))
						throw std::runtime_error("Failed to parse diffuse sphere #" + boost::lexical_cast<std::string>(i));

					s->matType = MATTE;
					s->matte.c.x = boost::lexical_cast<float>(sphereArgs[9]);
					s->matte.c.y = boost::lexical_cast<float>(sphereArgs[10]);
					s->matte.c.z = boost::lexical_cast<float>(sphereArgs[11]);
					break;
				}
				case 1: {
					if ((sphereArgs.size() != 12))
						throw std::runtime_error("Failed to parse mirror sphere #" + boost::lexical_cast<std::string> (i));

					s->matType = MIRROR;
					s->mirror.c.x = boost::lexical_cast<float>(sphereArgs[9]);
					s->mirror.c.y = boost::lexical_cast<float>(sphereArgs[10]);
					s->mirror.c.z = boost::lexical_cast<float>(sphereArgs[11]);
					break;
				}
				case 2: {
					if ((sphereArgs.size() != 15))
						throw std::runtime_error("Failed to parse glass sphere #" + boost::lexical_cast<std::string>(i));

					s->matType = GLASS;
					s->glass.c.x = boost::lexical_cast<float>(sphereArgs[9]);
					s->glass.c.y = boost::lexical_cast<float>(sphereArgs[10]);
					s->glass.c.z = boost::lexical_cast<float>(sphereArgs[11]);
					s->glass.ior = boost::lexical_cast<float>(sphereArgs[12]);
					s->glass.sigmaS = boost::lexical_cast<float>(sphereArgs[13]);
					s->glass.sigmaA = boost::lexical_cast<float>(sphereArgs[14]);
					break;
				}
				case 3: {
					if ((sphereArgs.size() != 15))
						throw std::runtime_error("Failed to parse mattertranslucent sphere #" + boost::lexical_cast<std::string>(i));

					s->matType = MATTETRANSLUCENT;
					s->mattertranslucent.c.x = boost::lexical_cast<float>(sphereArgs[9]);
					s->mattertranslucent.c.y = boost::lexical_cast<float>(sphereArgs[10]);
					s->mattertranslucent.c.z = boost::lexical_cast<float>(sphereArgs[11]);
					s->mattertranslucent.transparency = boost::lexical_cast<float>(sphereArgs[12]);
					s->mattertranslucent.sigmaS = boost::lexical_cast<float>(sphereArgs[13]);
					s->mattertranslucent.sigmaA = boost::lexical_cast<float>(sphereArgs[14]);
					break;
				}
				case 4: {
					if ((sphereArgs.size() != 13))
						throw std::runtime_error("Failed to parse glossy sphere #" + boost::lexical_cast<std::string>(i));

					s->matType = GLOSSY;
					s->glossy.c.x = boost::lexical_cast<float>(sphereArgs[9]);
					s->glossy.c.y = boost::lexical_cast<float>(sphereArgs[10]);
					s->glossy.c.z = boost::lexical_cast<float>(sphereArgs[11]);
					s->glossy.exponent = boost::lexical_cast<float>(sphereArgs[12]);
					break;
				}
				case 5: {
					if ((sphereArgs.size() != 16))
						throw std::runtime_error("Failed to parse glossytranslucent sphere #" + boost::lexical_cast<std::string>(i));

					s->matType = GLOSSYTRANSLUCENT;
					s->glossytranslucent.c.x = boost::lexical_cast<float>(sphereArgs[9]);
					s->glossytranslucent.c.y = boost::lexical_cast<float>(sphereArgs[10]);
					s->glossytranslucent.c.z = boost::lexical_cast<float>(sphereArgs[11]);
					s->glossytranslucent.exponent = boost::lexical_cast<float>(sphereArgs[12]);
					s->glossytranslucent.transparency = boost::lexical_cast<float>(sphereArgs[13]);
					s->glossytranslucent.sigmaS = boost::lexical_cast<float>(sphereArgs[14]);
					s->glossytranslucent.sigmaA = boost::lexical_cast<float>(sphereArgs[15]);
					break;
				}
				default:
					throw std::runtime_error("Unknown material for sphere #" + boost::lexical_cast<std::string>(i));
			}
		}

		f.close();
	}

	void UpdateCamera() {
		vsub(camera.dir, camera.target, camera.orig);
		vnorm(camera.dir);

		const Vec up = {0.f, 1.f, 0.f};
		const float fov = (FLOAT_PI / 180.f) * 45.f;
		vxcross(camera.x, camera.dir, up);
		vnorm(camera.x);
		vsmul(camera.x, windowWidth * fov / windowHeight, camera.x);

		vxcross(camera.y, camera.x, camera.dir);
		vnorm(camera.y);
		vsmul(camera.y, fov, camera.y);
	}

	void ResizeFrameBuffer() {
		const unsigned int pixelCount = windowWidth * windowHeight;

		unsigned int *seeds = new unsigned int[pixelCount * 2];
		for (unsigned int i = 0; i < selectedDevices.size(); ++i) {
			cl::CommandQueue &oclQueue = deviceQueues[i];

			// Allocate the sample buffer
			AllocOCLBufferRW(i, &samplesBuff[i], pixelCount * sizeof(float) * 3,
					"SamplesBuffer (Device " + boost::lexical_cast<std::string>(i) + ")");

			// Allocate the frame buffer
			delete[] pixels[i];
			pixels[i] = new float[pixelCount * 3];
			std::fill(pixels[i], pixels[i] + pixelCount * 3, 0.f);

			// Allocate the seeds for random number generator
			AllocOCLBufferRW(i, &seedsBuff[i], pixelCount * sizeof(unsigned int) * 2,
					"SeedsBuffer (Device " + boost::lexical_cast<std::string>(i) + ")");

			for (unsigned int j = 0; j < pixelCount * 2; j++)
				seeds[j] = 2 + 2 * i * pixelCount + j;

			oclQueue.enqueueWriteBuffer(*seedsBuff[i],
					CL_TRUE,
					0,
					seedsBuff[i]->getInfo<CL_MEM_SIZE>(),
					seeds);
		}
		delete[] seeds;

		if (selectedDevices.size() == 1)
			AllocOCLBufferWO(0, &pixelsBuff, pixelCount * sizeof(float) * 3, "PixelsBuffer");
		else {
			delete[] mergedPixels;
			mergedPixels = new float[pixelCount * 3];
		}
	}

	void UpdateKernelsArgs() {
		for (unsigned int i = 0; i < selectedDevices.size(); ++i) {
			kernelsSmallPT[i]->setArg(0, *samplesBuff[i]);
			kernelsSmallPT[i]->setArg(1, *seedsBuff[i]);
			kernelsSmallPT[i]->setArg(2, *cameraBuff[i]);
			kernelsSmallPT[i]->setArg(3, (unsigned int)spheres.size());
			kernelsSmallPT[i]->setArg(4, *spheresBuff[i]);
			kernelsSmallPT[i]->setArg(5, windowWidth);
			kernelsSmallPT[i]->setArg(6, windowHeight);
		}

		if (selectedDevices.size() == 1) {
			kernelToneMapping->setArg(0, *samplesBuff[0]);
			kernelToneMapping->setArg(1, *pixelsBuff);
			kernelToneMapping->setArg(2, windowWidth);
			kernelToneMapping->setArg(3, windowHeight);
		}
	}

	void UpdateCameraBuffer() {
		for (unsigned int i = 0; i < selectedDevices.size(); ++i) {
			cl::CommandQueue &oclQueue = deviceQueues[i];
			oclQueue.enqueueWriteBuffer(*cameraBuff[i],
					CL_FALSE,
					0,
					cameraBuff[i]->getInfo<CL_MEM_SIZE>(),
					&camera);
		}
	}

	void UpdateSpheresBuffer() {
		for (unsigned int i = 0; i < selectedDevices.size(); ++i) {
			cl::CommandQueue &oclQueue = deviceQueues[i];
			oclQueue.enqueueWriteBuffer(*spheresBuff[i],
					CL_FALSE,
					0,
					spheresBuff[i]->getInfo<CL_MEM_SIZE>(),
					&spheres[0]);
		}
	}

	void PrintHelp() {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(0.f, 0.f, 0.5f, 0.5f);
		glRecti(50, 50, windowWidth - 50, windowHeight - 50);

		glColor3f(1.f, 1.f, 1.f);
		int fontOffset = windowHeight - 50 - 20;
		glRasterPos2i((windowWidth - glutBitmapLength(GLUT_BITMAP_9_BY_15, (unsigned char *)"Help & Devices")) / 2, fontOffset);
		PrintString(GLUT_BITMAP_9_BY_15, "Help & Devices");

        // Help
		fontOffset -= 30;
		PrintHelpString(60, fontOffset, "h", "toggle Help");
		fontOffset -= 17;
		PrintHelpString(60, fontOffset, "arrow Keys", "rotate camera left/right/up/down");
		fontOffset -= 17;
		PrintHelpString(60, fontOffset, "a and d", "move camera left and right");
		fontOffset -= 17;
		PrintHelpString(60, fontOffset, "w and s", "move camera forward and backward");
		fontOffset -= 17;
		PrintHelpString(60, fontOffset, "r and f", "move camera up and down");
		fontOffset -= 17;
		PrintHelpString(60, fontOffset, "PageUp and PageDown", "move camera target up and down");
		fontOffset -= 17;
		PrintHelpString(60, fontOffset, "+ and -", "to select next/previous object");
		fontOffset -= 17;
		PrintHelpString(60, fontOffset, "2, 3, 4, 5, 6, 8, 9", "to move selected object");
		fontOffset -= 17;
		PrintHelpString(60, fontOffset, "p", "save image.ppm");
		fontOffset -= 17;
		PrintHelpString(60, fontOffset, "space", "restart rendering");
		fontOffset -= 17;

		// Print device specific information
		glColor3f(1.f, .5f, 0.f);
		int offset = 60;
		for (unsigned int i = 0; i < selectedDevices.size(); ++i) {
			const size_t deviceIndex = selectedDevices.size() - i - 1;
			const std::string deviceString = boost::str(boost::format("  [%s][%.1fM Sample/sec]") %
				selectedDevices[deviceIndex].getInfo<CL_DEVICE_NAME>() % (sampleSec[deviceIndex] / 1000000.0));

			glRasterPos2i(70, offset);
			PrintString(GLUT_BITMAP_9_BY_15, deviceString.c_str());
			offset += 17;
		}
		glRasterPos2i(60, offset);
		PrintString(GLUT_BITMAP_9_BY_15, "Devices:");

		glDisable(GL_BLEND);
	}

	float Radiance2PixelFloat(const float x) const {
		// Very slow !
		// return powf(x, 1.f / 2.2f);

		const unsigned int index = std::max(std::min(
			static_cast<int>(floorf(GAMMA_TABLE_SIZE * x)),
			GAMMA_TABLE_SIZE - 1), 0);
		return gammaTable[index];
	}

	//--------------------------------------------------------------------------
	// Rendering thread related methods
	//--------------------------------------------------------------------------

	void StartRendering() {
		renderThreads.resize(selectedDevices.size());
		for (unsigned int i = 0; i < selectedDevices.size(); ++i)
			renderThreads[i] = new boost::thread(RenderThreadImpl, this, i);
	}

	void StopRendering() {
		for (unsigned int i = 0; i < selectedDevices.size(); ++i)
			renderThreads[i]->interrupt();
		for (unsigned int i = 0; i < selectedDevices.size(); ++i)
			renderThreads[i]->join();
	}

	static void RenderThreadImpl(SmallPTGPU *smallptgpu, const unsigned int threadIndex) {
		try {
			size_t globalThreads = smallptgpu->windowWidth * smallptgpu->windowHeight;
			if (globalThreads % smallptgpu->kernelsWorkGroupSize[threadIndex] != 0)
				globalThreads = (globalThreads / smallptgpu->kernelsWorkGroupSize[threadIndex] + 1) * smallptgpu->kernelsWorkGroupSize[threadIndex];

			unsigned int kernelIterations = 1;
			smallptgpu->sampleSec[threadIndex] = 0.0;
			smallptgpu->currentSample[threadIndex] = 0;
			while (!boost::this_thread::interruption_requested()) {
				const double startTime = WallClockTime();

				cl::CommandQueue &oclQueue = smallptgpu->deviceQueues[threadIndex];
				for (unsigned int i = 0; i < kernelIterations; ++i) {
					// Set kernel arguments
					smallptgpu->kernelsSmallPT[threadIndex]->setArg(7, smallptgpu->currentSample[threadIndex]++);

					// Enqueue a kernel run
					oclQueue.enqueueNDRangeKernel(*(smallptgpu->kernelsSmallPT[threadIndex]), cl::NullRange,
							cl::NDRange(globalThreads), cl::NDRange(smallptgpu->kernelsWorkGroupSize[threadIndex]));
				}

				if (smallptgpu->selectedDevices.size() == 1) {
					// Image tone mapping
					oclQueue.enqueueNDRangeKernel(*(smallptgpu->kernelToneMapping), cl::NullRange,
								cl::NDRange(globalThreads), cl::NDRange(smallptgpu->kernelsWorkGroupSize[threadIndex]));

					// Read back the result
					oclQueue.enqueueReadBuffer(
							*(smallptgpu->pixelsBuff),
							CL_TRUE,
							0,
							smallptgpu->pixelsBuff->getInfo<CL_MEM_SIZE>(),
							smallptgpu->pixels[0]);
				} else {
					// Read back the result
					oclQueue.enqueueReadBuffer(
							*(smallptgpu->samplesBuff[threadIndex]),
							CL_TRUE,
							0,
							smallptgpu->samplesBuff[threadIndex]->getInfo<CL_MEM_SIZE>(),
							smallptgpu->pixels[threadIndex]);
				}

				const double elapsedTime = WallClockTime() - startTime;

				// A simple trick to smooth sample/sec value
				const double k = 0.1;
				smallptgpu->sampleSec[threadIndex] = smallptgpu->sampleSec[threadIndex] * (1.0 - k) +
						k * (kernelIterations * smallptgpu->windowWidth * smallptgpu->windowHeight / elapsedTime);

				if (elapsedTime < 0.075) {
					// Too fast, increase the number of kernel iterations
					++kernelIterations;
				} else if (elapsedTime > 0.1) {
					// Too slow, decrease the number of kernel iterations
					kernelIterations = std::max(kernelIterations - 1u, 1u);
				}
			}
		} catch (cl::Error err) {
			OCLTOY_LOG("RenderThreadImpl OpenCL ERROR: " << err.what() << "(" << OCLErrorString(err.err()) << ")");
		} catch (std::runtime_error err) {
			OCLTOY_LOG("RenderThreadImpl RUNTIME ERROR: " << err.what());
		} catch (std::exception err) {
			OCLTOY_LOG("RenderThreadImpl ERROR: " << err.what());
		}
	}

	std::vector<cl::Buffer *> samplesBuff;
	std::vector<cl::Buffer *> seedsBuff;
	std::vector<cl::Buffer *> cameraBuff;
	std::vector<cl::Buffer *> spheresBuff;

	std::vector<cl::Kernel *> kernelsSmallPT;
	std::vector<size_t> kernelsWorkGroupSize;
	// This kernel is compiled and used only if one single device has been selected
	cl::Kernel *kernelToneMapping;

	float gammaTable[GAMMA_TABLE_SIZE];
	std::vector<float *> pixels;
	// Used only when one single device is selected
	cl::Buffer *pixelsBuff;
	// Used only when multiple devices are selected
	float *mergedPixels;

	Camera camera;
	std::vector<Sphere> spheres;
	unsigned int maxDepth;
	float defaultVolumeSigmaS, defaultVolumeSigmaA;

	unsigned int currentSphere;

	// Thread statistics
	std::vector<double> sampleSec;
	std::vector<unsigned int> currentSample;

	std::vector<boost::thread *> renderThreads;
};

int main(int argc, char **argv) {
	SmallPTGPU toy;
	return toy.Run(argc, argv);
}
