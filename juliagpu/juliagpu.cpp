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

#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>

#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include "renderconfig.h"

class JuliaGPU : public OCLToy {
public:
	JuliaGPU() : OCLToy("JuliaGPU v" OCLTOYS_VERSION_MAJOR "." OCLTOYS_VERSION_MINOR " (OCLToys: http://code.google.com/p/ocltoys)"),
			mouseButton0(false), mouseButton2(false), shiftMouseButton0(false), muMouseButton0(false),
			mouseGrabLastX(0), mouseGrabLastY(0),
			pixels(NULL), pixelsBuff(NULL), configBuff(NULL), workGroupSize(64) {
		config.width = windowWidth;
		config.height = windowHeight;
		config.enableShadow = 1;
		config.superSamplingSize = 2;

		config.activateFastRendering = 1;
		config.maxIterations = 9;
		config.epsilon = 0.003f * 0.75f;

		config.light[0] = 5.f;
		config.light[1] = 10.f;
		config.light[2] = 15.f;

		config.mu[0] = -0.2f;
		config.mu[1] = 0.4f;
		config.mu[2] = -0.4f;
		config.mu[3] = -0.4f;

		vinit(config.camera.orig, 1.f, 2.f, 8.f);
		vinit(config.camera.target, 0.f, 0.f, 0.f);

		UpdateCamera();

		millisTimerFunc = 500;
		lastUserInputTime = WallClockTime();
	}

	virtual ~JuliaGPU() {
		FreeBuffers();
	}

protected:
	boost::program_options::options_description GetOptionsDescriction() {
		boost::program_options::options_description opts("JuliaGPU options");

		opts.add_options()
			("kernel,k", boost::program_options::value<std::string>()->default_value("preprocessed_rendering_kernel.cl"),
				"OpenCL kernel file name")
			("workgroupsize,z", boost::program_options::value<size_t>(), "OpenCL workgroup size");

		return opts;
	}

	virtual int RunToy() {
		SetUpOpenCL();
		UpdateJulia();

		glutMainLoop();

		return EXIT_SUCCESS;
	}

	void UpdateCamera() {
		vsub(config.camera.dir, config.camera.target, config.camera.orig);
		vnorm(config.camera.dir);

		const Vec up = { 0.f, 1.f, 0.f };
		vxcross(config.camera.x, config.camera.dir, up);
		vnorm(config.camera.x);
		vsmul(config.camera.x, config.width * .5135f / config.height, config.camera.x);

		vxcross(config.camera.y, config.camera.x, config.camera.dir);
		vnorm(config.camera.y);
		vsmul(config.camera.y, .5135f, config.camera.y);
	}

	//--------------------------------------------------------------------------
	// GLUT related code
	//--------------------------------------------------------------------------

#define MU_RECT_SIZE 80
	void DrawJulia(const int origX, const int origY, const float cR, const float cI) {
		float buffer[MU_RECT_SIZE][MU_RECT_SIZE][4];
		const float invSize = 3.f / MU_RECT_SIZE;
		int i, j;
		for (j = 0; j < MU_RECT_SIZE; ++j) {
			for (i = 0; i < MU_RECT_SIZE; ++i) {
				float x = i * invSize - 1.5f;
				float y = j * invSize - 1.5f;

				int iter;
				for (iter = 0; iter < 64; ++iter) {
					const float x2 = x * x;
					const float y2 = y * y;
					if (x2 + y2 > 4.f)
						break;

					const float newx = x2 - y2 +cR;
					const float newy = 2.f * x * y + cI;
					x = newx;
					y = newy;
				}

				buffer[i][j][0] = iter / 64.f;
				buffer[i][j][1] = 0.f;
				buffer[i][j][2] = 0.f;
				buffer[i][j][3] = 0.5f;
			}
		}

		glRasterPos2i(origX, origY);
		glDrawPixels(MU_RECT_SIZE, MU_RECT_SIZE, GL_RGBA, GL_FLOAT, buffer);
	}

	virtual void DisplayCallBack() {
		UpdateJulia();

		glRasterPos2i(0, 0);
		glDrawPixels(config.width, config.height, GL_RGB, GL_FLOAT, pixels);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(0.f, 0.f, 0.f, 0.8f);
		glRecti(0, windowHeight - 15,
				windowWidth - 1, windowHeight - 1);
		glDisable(GL_BLEND);

		// Title
		glColor3f(1.f, 1.f, 1.f);
		glRasterPos2i(4, windowHeight - 10);
		PrintString(GLUT_BITMAP_8_BY_13, windowTitle.c_str());

		// Caption line 0
		glRasterPos2i(4, 5);
		PrintString(GLUT_BITMAP_8_BY_13, captionString.c_str());

		// Caption line 1
		char captionString2[256];
		sprintf(captionString2, "Shadow/AO %d - SuperSampling %dx%d - Fast rendering (%s)",
				config.enableShadow, config.superSamplingSize, config.superSamplingSize,
				config.activateFastRendering ? "active" : "not active");
		glRasterPos2i(4, 20);
		PrintString(GLUT_BITMAP_8_BY_13, captionString2);
		// Caption line 2
		sprintf(captionString2, "Epsilon %.5f - Max. Iter. %u",
				config.epsilon, config.maxIterations);
		glRasterPos2i(4, 35);
		PrintString(GLUT_BITMAP_8_BY_13, captionString2);
		// Caption line 3
		sprintf(captionString2, "Mu = (%.3f, %.3f, %.3f, %.3f)",
				config.mu[0], config.mu[1], config.mu[2], config.mu[3]);
		glRasterPos2i(4, 50);
		PrintString(GLUT_BITMAP_8_BY_13, captionString2);

		// Draw Mu constants
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		const int baseMu1 = config.width - MU_RECT_SIZE - 2;
		const int baseMu2 = 1;
		DrawJulia(baseMu1, baseMu2, config.mu[0], config.mu[1]);
		const int baseMu3 = config.width - MU_RECT_SIZE - 2;
		const int baseMu4 = MU_RECT_SIZE + 2;
		DrawJulia(baseMu3, baseMu4, config.mu[2], config.mu[3]);
		glDisable(GL_BLEND);

		glColor3f(1.f, 1.f, 1.f);
		const int mu1 = (int)(baseMu1 + MU_RECT_SIZE * (config.mu[0] + 1.5f) / 3.f);
		const int mu2 = (int)(baseMu2 + MU_RECT_SIZE * (config.mu[1] + 1.5f) / 3.f);
		glBegin(GL_LINES);
		glVertex2i(mu1 - 4, mu2);
		glVertex2i(mu1 + 4, mu2);
		glVertex2i(mu1, mu2 - 4);
		glVertex2i(mu1, mu2 + 4);
		glEnd();

		const int mu3 = (int)(baseMu3 + MU_RECT_SIZE * (config.mu[2] + 1.5f) / 3.f);
		const int mu4 = (int)(baseMu4 + MU_RECT_SIZE * (config.mu[3] + 1.5f) / 3.f);
		glBegin(GL_LINES);
		glVertex2i(mu3 - 4, mu4);
		glVertex2i(mu3 + 4, mu4);
		glVertex2i(mu3, mu4 - 4);
		glVertex2i(mu3, mu4 + 4);
		glEnd();

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

	void UpdateConfig(const bool allocateBuffers = false) {
		if (allocateBuffers)
			AllocateBuffers();

		UpdateCamera();

		config.activateFastRendering = 1;
	}

	//--------------------------------------------------------------------------
	// GLUT call backs
	//--------------------------------------------------------------------------

	virtual void ReshapeCallBack(int newWidth, int newHeight) {
		windowWidth = newWidth;
		windowHeight = newHeight;

		glViewport(0, 0, windowWidth, windowHeight);
		glLoadIdentity();
		glOrtho(-.5f, windowWidth - .5f,
				-.5f, windowHeight - .5f, -1.f, 1.f);

		config.width = windowWidth;
		config.height = newHeight;
		UpdateConfig(true);

		glutPostRedisplay();
		lastUserInputTime = WallClockTime();
	}

#define MOVE_STEP 0.5f
#define ROTATE_STEP (2.f * ((float)M_PI) / 180.f)
	virtual void KeyCallBack(unsigned char key, int x, int y) {
		bool needRedisplay = true;

		switch (key) {
			case 'p': {
				// Write image to PPM file
				std::ofstream f("image.ppm", std::ofstream::trunc);
				if (!f.good()) {
					OCLTOY_LOG("Failed to open image file: image.ppm");
				} else {
					f << "P3" << std::endl;
					f << config.width << " " << config.height << std::endl;
					f << "255" << std::endl;

					for (int y = (int)config.height - 1; y >= 0; --y) {
						const float *p = &pixels[y * config.width * 3];
						for (int x = 0; x < (int)config.width; ++x) {
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
				OCLTOY_LOG("Done");
				exit(EXIT_SUCCESS);
				break;
			case ' ': // Refresh display
				break;
			case 'h':
				printHelp = (!printHelp);
				break;
			case 'a': {
				Vec dir = config.camera.x;
				vnorm(dir);
				vsmul(dir, -MOVE_STEP, dir);
				vadd(config.camera.orig, config.camera.orig, dir);
				vadd(config.camera.target, config.camera.target, dir);
				break;
			}
			case 'd': {
				Vec dir = config.camera.x;
				vnorm(dir);
				vsmul(dir, MOVE_STEP, dir);
				vadd(config.camera.orig, config.camera.orig, dir);
				vadd(config.camera.target, config.camera.target, dir);
				break;
			}
			case 'w': {
				Vec dir = config.camera.dir;
				vsmul(dir, MOVE_STEP, dir);
				vadd(config.camera.orig, config.camera.orig, dir);
				vadd(config.camera.target, config.camera.target, dir);
				break;
			}
			case 's': {
				Vec dir = config.camera.dir;
				vsmul(dir, -MOVE_STEP, dir);
				vadd(config.camera.orig, config.camera.orig, dir);
				vadd(config.camera.target, config.camera.target, dir);
				break;
			}
			case 'r':
				config.camera.orig.y += MOVE_STEP;
				config.camera.target.y += MOVE_STEP;
				break;
			case 'f':
				config.camera.orig.y -= MOVE_STEP;
				config.camera.target.y -= MOVE_STEP;
				break;
			case 'l':
				config.enableShadow = (!config.enableShadow);
				break;
			case '1':
				config.epsilon *= 0.75f;
				break;
			case '2':
				config.epsilon *= 1.f / 0.75f;
				break;
			case '3':
				config.maxIterations = std::max(1u, config.maxIterations - 1u);
				break;
			case '4':
				config.maxIterations = std::min(12u, config.maxIterations + 1u);
				break;
			case '5':
				config.superSamplingSize = std::max(1u, config.superSamplingSize - 1u);
				break;
			case '6':
				config.superSamplingSize = std::min(5u, config.superSamplingSize + 1u);
				break;
			default:
				needRedisplay = false;
				break;
		}

		if (needRedisplay) {
			UpdateConfig();
			glutPostRedisplay();
			lastUserInputTime = WallClockTime();
		}
	}

	void RotateLightX(const float k) {
		const float y = config.light[1];
		const float z = config.light[2];
		config.light[1] = y * cosf(k) + z * sinf(k);
		config.light[2] = -y * sinf(k) + z * cosf(k);
	}

	void RotateLightY(const float k) {
		const float x = config.light[0];
		const float z = config.light[2];
		config.light[0] = x * cosf(k) - z * sinf(k);
		config.light[2] = x * sinf(k) + z * cosf(k);
	}

	void RotateCameraXbyOrig(const float k) {
		Vec t = config.camera.orig;
		config.camera.orig.y = t.y * cosf(k) + t.z * sinf(k);
		config.camera.orig.z = -t.y * sinf(k) + t.z * cosf(k);
	}

	void RotateCameraYbyOrig(const float k) {
		Vec t = config.camera.orig;
		config.camera.orig.x = t.x * cosf(k) - t.z * sinf(k);
		config.camera.orig.z = t.x * sinf(k) + t.z * cosf(k);
	}

	void RotateCameraX(const float k) {
		Vec t = config.camera.target;
		vsub(t, t, config.camera.orig);
		t.y = t.y * cosf(k) + t.z * sinf(k);
		t.z = -t.y * sinf(k) + t.z * cosf(k);
		vadd(t, t, config.camera.orig);
		config.camera.target = t;
	}

	void RotateCameraY(const float k) {
		Vec t = config.camera.target;
		vsub(t, t, config.camera.orig);
		t.x = t.x * cosf(k) - t.z * sinf(k);
		t.z = t.x * sinf(k) + t.z * cosf(k);
		vadd(t, t, config.camera.orig);
		config.camera.target = t;
	}

	void SpecialCallBack(int key, int x, int y) {
		bool needRedisplay = true;

		switch (key) {
			case GLUT_KEY_UP:
				RotateCameraX(-ROTATE_STEP);
				break;
			case GLUT_KEY_DOWN:
				RotateCameraX(ROTATE_STEP);
				break;
			case GLUT_KEY_LEFT:
				RotateCameraY(-ROTATE_STEP);
				break;
			case GLUT_KEY_RIGHT:
				RotateCameraY(ROTATE_STEP);
				break;
			case GLUT_KEY_PAGE_UP:
				config.camera.target.y += MOVE_STEP;
				break;
			case GLUT_KEY_PAGE_DOWN:
				config.camera.target.y -= MOVE_STEP;
				break;
			default:
				needRedisplay = false;
				break;
		}

		if (needRedisplay) {
			UpdateConfig();
			glutPostRedisplay();
			lastUserInputTime = WallClockTime();
		}
	}

	void MouseCallBack(int button, int state, int x, int y) {
		if (button == 0) {
			if (state == GLUT_DOWN) {
				// Record start position
				mouseGrabLastX = x;
				mouseGrabLastY = y;
				mouseButton0 = true;

				int mod = glutGetModifiers();
				if (mod == GLUT_ACTIVE_SHIFT)
					shiftMouseButton0 = true;
				else {
					shiftMouseButton0 = false;

					const int ry = config.height - y - 1;
					const int baseMu1 = config.width - MU_RECT_SIZE - 2;
					const int baseMu2 = 1;
					const int baseMu3 = config.width - MU_RECT_SIZE - 2;
					const int baseMu4 = MU_RECT_SIZE + 2;

					if ((x >= baseMu1 && x <= baseMu1 + MU_RECT_SIZE) &&
							(ry >= baseMu2 && ry <= baseMu2 + MU_RECT_SIZE)) {
						muMouseButton0 = true;
						config.mu[0] = 3.f * (x - baseMu1) / (float) MU_RECT_SIZE - 1.5f;
						config.mu[1] = 3.f * (ry - baseMu2) / (float) MU_RECT_SIZE - 1.5f;

						UpdateConfig();
						glutPostRedisplay();
						lastUserInputTime = WallClockTime();
					} else if ((x >= baseMu3 && x <= baseMu3 + MU_RECT_SIZE) &&
							(ry >= baseMu4 && ry <= baseMu4 + MU_RECT_SIZE)) {
						muMouseButton0 = true;
						config.mu[2] = 3.f * (x - baseMu3) / (float) MU_RECT_SIZE - 1.5f;
						config.mu[3] = 3.f * (ry - baseMu4) / (float) MU_RECT_SIZE - 1.5f;

						UpdateConfig();
						glutPostRedisplay();
						lastUserInputTime = WallClockTime();
					} else
						muMouseButton0 = false;
				}
			} else if (state == GLUT_UP) {
				mouseButton0 = false;
				shiftMouseButton0 = false;
				muMouseButton0 = false;
			}
		} else if (button == 2) {
			if (state == GLUT_DOWN) {
				// Record start position
				mouseGrabLastX = x;
				mouseGrabLastY = y;
				mouseButton2 = true;
			} else if (state == GLUT_UP) {
				mouseButton2 = false;
			}
		}
	}

	virtual void MotionCallBack(int x, int y) {
		bool needRedisplay = true;

		if (mouseButton0) {
			const int ry = config.height - y - 1;
			const int baseMu1 = config.width - MU_RECT_SIZE - 2;
			const int baseMu2 = 1;
			const int baseMu3 = config.width - MU_RECT_SIZE - 2;
			const int baseMu4 = MU_RECT_SIZE + 2;

			// Check if the click was over first Mu red rectangle
			if (muMouseButton0 && (x >= baseMu1 && x <= baseMu1 + MU_RECT_SIZE) &&
					(ry >= baseMu2 && ry <= baseMu2 + MU_RECT_SIZE)) {
				config.mu[0] = 3.f * (x - baseMu1) / (float)MU_RECT_SIZE - 1.5f;
				config.mu[1] = 3.f * (ry - baseMu2) / (float)MU_RECT_SIZE - 1.5f;
			} else if (muMouseButton0 && (x >= baseMu3 && x <= baseMu3 + MU_RECT_SIZE) &&
					(ry >= baseMu4 && ry <= baseMu4 + MU_RECT_SIZE)) {
				config.mu[2] = 3.f * (x - baseMu3) / (float)MU_RECT_SIZE - 1.5f;
				config.mu[3] = 3.f * (ry - baseMu4) / (float)MU_RECT_SIZE - 1.5f;
			} else if (!muMouseButton0) {
				const int distX = x - mouseGrabLastX;
				const int distY = y - mouseGrabLastY;

				if (!shiftMouseButton0) {
					vclr(config.camera.target);
					RotateCameraYbyOrig(0.2f * distX * ROTATE_STEP);
					RotateCameraXbyOrig(0.2f * distY * ROTATE_STEP);
				} else {
					RotateCameraY(0.1f * distX * ROTATE_STEP);
					RotateCameraX(0.1f * distY * ROTATE_STEP);
				}

				mouseGrabLastX = x;
				mouseGrabLastY = y;
			}
		} else if (mouseButton2) {
			const int distX = x - mouseGrabLastX;
			const int distY = y - mouseGrabLastY;

			RotateLightX(-0.2f * distY * ROTATE_STEP);
			RotateLightY(-0.2f * distX * ROTATE_STEP);

			mouseGrabLastX = x;
			mouseGrabLastY = y;
		} else
			needRedisplay = false;

		if (needRedisplay) {
			UpdateConfig();
			glutPostRedisplay();
			lastUserInputTime = WallClockTime();
		}
	}

	void TimerCallBack(int id) {
		// Check the time since last screen update
		const double elapsedTime = WallClockTime() - lastUserInputTime;

		if (elapsedTime > 1.0) {
			if (config.activateFastRendering) {
				// Enable supersampling
				config.activateFastRendering = 0;
				glutPostRedisplay();
			}
		} else
			config.activateFastRendering = 1;

		// Refresh the timer
		glutTimerFunc(millisTimerFunc, &OCLToy::GlutTimerFunc, 0);
	}

	//--------------------------------------------------------------------------
	// OpenCL related code
	//--------------------------------------------------------------------------

	virtual unsigned int GetMaxDeviceCountSupported() const { return 1; }

private:
	void SetUpOpenCL() {
		//----------------------------------------------------------------------
		// Allocate buffer
		//----------------------------------------------------------------------

		AllocateBuffers();

		//----------------------------------------------------------------------
		// Compile kernel
		//----------------------------------------------------------------------

		const std::string &kernelFileName = commandLineOpts["kernel"].as<std::string>();
		OCLTOY_LOG("Compile OpenCL kernel: " << kernelFileName);

		// Read the kernel
		const std::string kernelSource = ReadSources(kernelFileName, "juliagpu");

		// Create the kernel program
		cl::Device &oclDevice = selectedDevices[0];
		cl::Context &oclContext = deviceContexts[0];
		cl::Program::Sources source(1, std::make_pair(kernelSource.c_str(), kernelSource.length()));
		cl::Program program = cl::Program(oclContext, source);
		try {
			VECTOR_CLASS<cl::Device> buildDevice;
			buildDevice.push_back(oclDevice);
			program.build(buildDevice, "-I. -I../common");
		} catch (cl::Error err) {
			cl::STRING_CLASS strError = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(oclDevice);
			OCLTOY_LOG("Kernel compilation error:\n" << strError.c_str());

			throw err;
		}

		kernelJulia = cl::Kernel(program, "JuliaGPU");
		kernelJulia.getWorkGroupInfo<size_t>(oclDevice, CL_KERNEL_WORK_GROUP_SIZE, &workGroupSize);
		if (commandLineOpts.count("workgroupsize"))
			workGroupSize = commandLineOpts["workgroupsize"].as<size_t>();
		OCLTOY_LOG("Using workgroup size: " << workGroupSize);
	}

	void FreeBuffers() {
		FreeOCLBuffer(0, &pixelsBuff);
		delete[] pixels;

		FreeOCLBuffer(0, &configBuff);
	}

	void AllocateBuffers() {
		delete[] pixels;
		const size_t size = config.width * config.height;
		pixels = new float[size * 3];
		std::fill(&pixels[0], &pixels[size * 3], 0.f);

		AllocOCLBufferWO(0, &pixelsBuff, size * sizeof(float) * 3, "FrameBuffer");

		AllocOCLBufferRO(0, &configBuff, &config, sizeof(RenderingConfig), "RenderingConfig");
	}

	void UpdateJulia() {
		const double startTime = WallClockTime();

		// Send the new configuration to the OpenCL device
		cl::CommandQueue &oclQueue = deviceQueues[0];
		oclQueue.enqueueWriteBuffer(*configBuff, CL_FALSE, 0, configBuff->getInfo<CL_MEM_SIZE>(), &config);

		// Set kernel arguments
		kernelJulia.setArg(0, *pixelsBuff);
		kernelJulia.setArg(1, *configBuff);

		// Enqueue a kernel run
		size_t globalThreads = config.width * config.height;
		if (globalThreads % workGroupSize != 0)
			globalThreads = (globalThreads / workGroupSize + 1) * workGroupSize;

		if (!config.activateFastRendering && (config.superSamplingSize > 1)) {
			kernelJulia.setArg(3, config.superSamplingSize * config.superSamplingSize);

			int x, y;
			for (y = 0; y < config.superSamplingSize; ++y) {
				for (x = 0; x < config.superSamplingSize; ++x) {
					const float sampleX = (x + .5f) / config.superSamplingSize;
					const float sampleY = (y + .5f) / config.superSamplingSize;

					// First pass
					if ((x == 0) && (y == 0))
						kernelJulia.setArg(2, 0);
					else
						kernelJulia.setArg(2, 1);
					kernelJulia.setArg(4, sampleX);
					kernelJulia.setArg(5, sampleY);

					oclQueue.enqueueNDRangeKernel(kernelJulia, cl::NullRange,
							cl::NDRange(globalThreads), cl::NDRange(workGroupSize));
				}
			}
		} else {
			kernelJulia.setArg(2, 0);
			kernelJulia.setArg(3, 1);
			kernelJulia.setArg(4, 0.f);
			kernelJulia.setArg(5, 0.f);

			oclQueue.enqueueNDRangeKernel(kernelJulia, cl::NullRange,
					cl::NDRange(globalThreads), cl::NDRange(workGroupSize));
		}

		// Read back the result
		oclQueue.enqueueReadBuffer(
				*pixelsBuff,
				CL_TRUE,
				0,
				pixelsBuff->getInfo<CL_MEM_SIZE>(),
				pixels);

		const double elapsedTime = WallClockTime() - startTime;
		double sampleSec = config.width * config.height / elapsedTime;
		if (!config.activateFastRendering && (config.superSamplingSize > 1))
			sampleSec *= config.superSamplingSize * config.superSamplingSize;
		captionString = boost::str(boost::format("Rendering time: %.3f secs (Sample/sec %.1fK)") %
			elapsedTime % (sampleSec / 1000.0));
	}

	void PrintHelp() {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(0.f, 0.f, 0.5f, 0.5f);
		glRecti(50, 50, windowWidth - 50, windowHeight - 50);

		glColor3f(1.f, 1.f, 1.f);
		int fontOffset = windowHeight - 50 - 20;
		glRasterPos2i((windowWidth - glutBitmapLength(GLUT_BITMAP_9_BY_15, (unsigned char *)"Help")) / 2, fontOffset);
		PrintString(GLUT_BITMAP_9_BY_15, "Help");

		fontOffset -= 30;
		PrintHelpString(60, fontOffset, "h", "toggle Help");
		fontOffset -= 17;
		PrintHelpString(60, fontOffset, "arrow Keys", "rotate camera");
		fontOffset -= 17;
		PrintHelpString(60, fontOffset, "Mouse button 0 + Mouse X, Y", "rotate camera around the center");
		fontOffset -= 17;
		PrintHelpString(60, fontOffset, "Shift + Mouse button 0 + Mouse X, Y", "rotate camera");
		fontOffset -= 17;
		PrintHelpString(60, fontOffset, "Mouse button 2 + Mouse X, Y", "rotate light");
		fontOffset -= 17;
		PrintHelpString(60, fontOffset, "a, s, d, w", "move camera");
		fontOffset -= 17;
		PrintHelpString(60, fontOffset, "1, 2", "decrease, increase epsilon");
		fontOffset -= 17;
		PrintHelpString(60, fontOffset, "3, 4", "decrease, increase max. iterations");
		fontOffset -= 17;
		PrintHelpString(60, fontOffset, "5, 6", "decrease, increase samples per pixel");
		fontOffset -= 17;
		PrintHelpString(60, fontOffset, "Mouse button 0 on red rectangles", "change Mu values");
		fontOffset -= 17;
		PrintHelpString(60, fontOffset, "l", "toggle shadow/AO");
		fontOffset -= 17;
		PrintHelpString(60, fontOffset, "p", "save image.ppm");

		glDisable(GL_BLEND);
	}

	bool mouseButton0, mouseButton2, shiftMouseButton0, muMouseButton0;
	int mouseGrabLastX, mouseGrabLastY;
	double lastUserInputTime;

	float *pixels;
	cl::Buffer *pixelsBuff;

	RenderingConfig config;
	cl::Buffer *configBuff;

	cl::Kernel kernelJulia;
	size_t workGroupSize;

	std::string captionString;
};

int main(int argc, char **argv) {
	JuliaGPU toy;
	return toy.Run(argc, argv);
}
