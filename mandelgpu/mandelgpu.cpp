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

#include "ocltoy.h"

#include <iostream>
#include <fstream>
#include <string>

#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

class MandelGPU : public OCLToy {
public:
	MandelGPU() : OCLToy("MandelGPU v" OCLTOYS_VERSION_MAJOR "." OCLTOYS_VERSION_MINOR " (OCLToys: http://code.google.com/p/ocltoys)"),
			scale(3.5f), offsetX(-.5f), offsetY(0.f), maxIterations(256),
			mouseButton0(false), mouseButton2(false), mouseGrabLastX(0), mouseGrabLastY(0),
			pixels(NULL), pixelsBuff(NULL), workGroupSize(64) {
	}
	virtual ~MandelGPU() {
		FreeBuffers();
	}

protected:
	boost::program_options::options_description GetOptionsDescriction() {
		boost::program_options::options_description opts("MandelGPU options");

		opts.add_options()
			("kernel,k", boost::program_options::value<std::string>()->default_value("rendering_kernel_float4.cl"),
				"OpenCL kernel file name")
			("iterations,i", boost::program_options::value<unsigned int>()->default_value(256u),
				"Number of iterations")
			("workgroupsize,z", boost::program_options::value<size_t>(), "OpenCL workgroup size");

		return opts;
	}

	virtual int RunToy() {
		maxIterations = commandLineOpts["iterations"].as<unsigned int>();

		SetUpOpenCL();
		UpdateMandel();

		glutMainLoop();

		return EXIT_SUCCESS;
	}

	//--------------------------------------------------------------------------
	// GLUT related code
	//--------------------------------------------------------------------------

	virtual void DisplayCallBack() {
		UpdateMandel();

		glRasterPos2i(0, 0);
		glDrawPixels(windowWidth, windowHeight, GL_LUMINANCE, GL_UNSIGNED_BYTE, pixels);

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

	virtual void ReshapeCallBack(int newWidth, int newHeight) {
		// Width must be a multiple of 4
		windowWidth = newWidth;
		if (windowWidth % 4 != 0)
			windowWidth = (windowWidth / 4 + 1) * 4;
		windowHeight = newHeight;

		glViewport(0, 0, windowWidth, windowHeight);
		glLoadIdentity();
		glOrtho(-.5f, windowWidth - .5f,
				-.5f, windowHeight - .5f, -1.f, 1.f);

		AllocateBuffers();

		glutPostRedisplay();
	}

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
					f << windowWidth << " " << windowHeight << std::endl;
					f << "255" << std::endl;

					for (int y = windowHeight - 1; y >= 0; --y) {
						const unsigned char *p = (unsigned char *)(&pixels[y * windowWidth / 4]);
						for (int x = 0; x < windowWidth; ++x, p++) {
							const std::string value = boost::lexical_cast<std::string>((unsigned int)(*p));
							f << value << " " << value << " " << value << std::endl;
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
			case '+':
				maxIterations += 32;
				break;
			case '-':
				maxIterations -= 32;
				break;
			case ' ': // Refresh display
				break;
			case 'h':
				printHelp = (!printHelp);
				break;
			default:
				needRedisplay = false;
				break;
		}

		if (needRedisplay) {
			UpdateMandel();
			glutPostRedisplay();
		}
	}

	void SpecialCallBack(int key, int x, int y) {
#define SCALE_STEP (0.1f)
#define OFFSET_STEP (0.025f)
		bool needRedisplay = true;

		switch (key) {
			case GLUT_KEY_UP:
				offsetY += scale * OFFSET_STEP;
				break;
			case GLUT_KEY_DOWN:
				offsetY -= scale * OFFSET_STEP;
				break;
			case GLUT_KEY_LEFT:
				offsetX -= scale * OFFSET_STEP;
				break;
			case GLUT_KEY_RIGHT:
				offsetX += scale * OFFSET_STEP;
				break;
			case GLUT_KEY_PAGE_UP:
				scale *= 1.f - SCALE_STEP;
				break;
			case GLUT_KEY_PAGE_DOWN:
				scale *= 1.f + SCALE_STEP;
				break;
			default:
				needRedisplay = false;
				break;
		}

		if (needRedisplay) {
			UpdateMandel();
			glutPostRedisplay();
		}
	}

	void MouseCallBack(int button, int state, int x, int y) {
		if (button == 0) {
			if (state == GLUT_DOWN) {
				// Record start position
				mouseGrabLastX = x;
				mouseGrabLastY = y;
				mouseButton0 = true;
			} else if (state == GLUT_UP) {
				mouseButton0 = false;
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
			const int distX = x - mouseGrabLastX;
			const int distY = y - mouseGrabLastY;

			offsetX -= (40.f * distX / windowWidth) * scale * OFFSET_STEP;
			offsetY += (40.f * distY / windowHeight) * scale * OFFSET_STEP;

			mouseGrabLastX = x;
			mouseGrabLastY = y;
		} else if (mouseButton2) {
			const int distX = x - mouseGrabLastX;

			scale *= 1.0f - (2.f * distX / windowWidth);

			mouseGrabLastX = x;
			mouseGrabLastY = y;
		} else
			needRedisplay = false;

		if (needRedisplay)
			glutPostRedisplay();
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
		const std::string kernelSource = ReadSources(kernelFileName, "mandelgpu");

		// Create the kernel program
		cl::Device &oclDevice = selectedDevices[0];
		cl::Context &oclContext = deviceContexts[0];
		cl::Program::Sources source(1, std::make_pair(kernelSource.c_str(), kernelSource.length()));
		cl::Program program = cl::Program(oclContext, source);
		try {
			VECTOR_CLASS<cl::Device> buildDevice;
			buildDevice.push_back(oclDevice);
			program.build(buildDevice);
		} catch (cl::Error err) {
			cl::STRING_CLASS strError = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(oclDevice);
			OCLTOY_LOG("Kernel compilation error:\n" << strError.c_str());

			throw err;
		}

		kernelMandel = cl::Kernel(program, "mandelGPU");
		kernelMandel.getWorkGroupInfo<size_t>(oclDevice, CL_KERNEL_WORK_GROUP_SIZE, &workGroupSize);
		if (commandLineOpts.count("workgroupsize"))
			workGroupSize = commandLineOpts["workgroupsize"].as<size_t>();
		OCLTOY_LOG("Using workgroup size: " << workGroupSize);
	}

	void FreeBuffers() {
		FreeOCLBuffer(0, &pixelsBuff);
		delete[] pixels;
	}

	void AllocateBuffers() {
		delete[] pixels;
		const int pixelCount = windowWidth * windowHeight;
		const size_t size = pixelCount / 4 + 1;
		pixels = new unsigned int[size];
		std::fill(&pixels[0], &pixels[size], 0);

		AllocOCLBufferWO(0, &pixelsBuff, size * sizeof(unsigned int), "FrameBuffer");
	}

	void UpdateMandel() {
		const double startTime = WallClockTime();

		// Set kernel arguments
		kernelMandel.setArg(0, *pixelsBuff);
		kernelMandel.setArg(1, windowWidth);
		kernelMandel.setArg(2, windowHeight);
		kernelMandel.setArg(3, scale);
		kernelMandel.setArg(4, offsetX);
		kernelMandel.setArg(5, offsetY);
		kernelMandel.setArg(6, maxIterations);

		// Enqueue a kernel run
		cl::CommandQueue &oclQueue = deviceQueues[0];
		// Each work item renders 4 pixels
		const size_t workItemCount = RoundUp(windowWidth * windowHeight, 4) / 4;
		const size_t globalThreads = RoundUp(workItemCount, workGroupSize);
		oclQueue.enqueueNDRangeKernel(kernelMandel, cl::NullRange,
				cl::NDRange(globalThreads), cl::NDRange(workGroupSize));

		// Read back the result
		oclQueue.enqueueReadBuffer(
				*pixelsBuff,
				CL_TRUE,
				0,
				pixelsBuff->getInfo<CL_MEM_SIZE>(),
				pixels);

		const double elapsedTime = WallClockTime() - startTime;
		const double sampleSec = windowHeight * windowWidth / elapsedTime;
		captionString = boost::str(boost::format("Rendering time: %.3f secs (Sample/sec %.1fK Max. Iterations %d)") %
			elapsedTime % (sampleSec / 1000.0) % maxIterations);
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
		PrintHelpString(60, fontOffset, "Arrow Keys", "move left/right/up/down");
		fontOffset -= 17;
		PrintHelpString(60, fontOffset, "PageUp and PageDown", "zoom in/out");
		fontOffset -= 17;
		PrintHelpString(60, fontOffset, "Mouse button 0 + Mouse X, Y", "move left/right/up/down");
		fontOffset -= 17;
		PrintHelpString(60, fontOffset, "Mouse button 2 + Mouse X", "zoom in/out");
		fontOffset -= 17;
		PrintHelpString(60, fontOffset, "+", "increase the max. iterations by 32");
		fontOffset -= 17;
		PrintHelpString(60, fontOffset, "-", "decrease the max. iterations by 32");
		fontOffset -= 17;
		PrintHelpString(60, fontOffset, "p", "save image.ppm");

		glDisable(GL_BLEND);
	}

	float scale, offsetX, offsetY;
	int maxIterations;
	bool mouseButton0, mouseButton2;
	int mouseGrabLastX, mouseGrabLastY;

	unsigned int *pixels;
	cl::Buffer *pixelsBuff;

	cl::Kernel kernelMandel;
	size_t workGroupSize;

	std::string captionString;
};

int main(int argc, char **argv) {
	MandelGPU toy;
	return toy.Run(argc, argv);
}
