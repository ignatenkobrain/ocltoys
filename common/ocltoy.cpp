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

#include <iostream>
#include <stdexcept>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

#include "ocltoy.h"

//------------------------------------------------------------------------------

#if defined(__GNUC__) && !defined(__CYGWIN__)
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <typeinfo>
#include <cxxabi.h>

static std::string Demangle(const char *symbol) {
	size_t size;
	int status;
	char temp[128];
	char* result;

	if (1 == sscanf(symbol, "%*[^'(']%*[^'_']%[^')''+']", temp)) {
		if (NULL != (result = abi::__cxa_demangle(temp, NULL, &size, &status))) {
			std::string r = result;
			return r + " [" + symbol + "]";
		}
	}

	if (1 == sscanf(symbol, "%127s", temp))
		return temp;

	return symbol;
}

void OCLToyTerminate(void) {
	OCLTOY_LOG("=========================================================");
	OCLTOY_LOG("Unhandled exception");

	void *array[32];
	size_t size = backtrace(array, 32);
	char **strings = backtrace_symbols(array, size);

	OCLTOY_LOG("Obtained " << size << " stack frames.");

	for (size_t i = 0; i < size; i++)
		OCLTOY_LOG("  " << Demangle(strings[i]));

	free(strings);
}
#endif

//------------------------------------------------------------------------------

OCLToy *OCLToy::currentOCLToy = NULL;

void OCLToyDebugHandler(const char *msg) {
	std::cerr << "[OCLToy] " << msg << std::endl;
}

OCLToy::OCLToy(const std::string &winTitle) : windowTitle(winTitle),
		windowWidth(800), windowHeight(600), millisTimerFunc(0), useIdleCallback(false),
		printHelp(true) {
	currentOCLToy = this;
}

int OCLToy::Run(int argc, char **argv) {
#if defined(__GNUC__) && !defined(__CYGWIN__)
	std::set_terminate(OCLToyTerminate);
#endif

	try {
		OCLTOY_LOG(windowTitle);

		//----------------------------------------------------------------------
		// Parse command line options
		//----------------------------------------------------------------------

		glutInit(&argc, argv);

		boost::program_options::options_description genericOpts("Generic options");
		genericOpts.add_options()
			("help,h", "Display this help and exit")
			("width,w", boost::program_options::value<int>()->default_value(800), "Window width")
			("height,e", boost::program_options::value<int>()->default_value(600), "Window height")
			("directory,d", boost::program_options::value<std::string>(), "Current directory path")
			("ocldevices,o", boost::program_options::value<std::string>()->default_value("ALL_GPUS"),
				"OpenCL device selection string. It can be ALL, ALL_GPUS, ALL_CPUS, FIRST_GPU, FIRST_CPU or a "
				"binary string where 0 means disabled and 1 enabled (for instance, 1100 will use only the first "
				"and second devices of the 4 available). NOTE: OpenCL accelerators are considered GPUs.")
			("noscreenhelp,s", "Disable on screen help");

		boost::program_options::options_description toyOpts = GetOptionsDescriction();

		boost::program_options::options_description opts;
		opts.add(genericOpts).add(toyOpts);

		try {
			// Disable guessing of option names
			const int cmdstyle = boost::program_options::command_line_style::default_style &
				~boost::program_options::command_line_style::allow_guessing;
			boost::program_options::store(boost::program_options::command_line_parser(argc, argv).
				style(cmdstyle).options(opts).run(), commandLineOpts);

			windowWidth = commandLineOpts["width"].as<int>();
			windowHeight = commandLineOpts["height"].as<int>();
			if (commandLineOpts.count("directory"))
				boost::filesystem::current_path(boost::filesystem::path(commandLineOpts["directory"].as<std::string>()));
			if (commandLineOpts.count("noscreenhelp"))
				printHelp = false;

			if (commandLineOpts.count("help")) {
				OCLTOY_LOG("Command usage" << std::endl << opts);
				exit(EXIT_SUCCESS);
			}
		} catch(boost::program_options::error &e) {
			OCLTOY_LOG("COMMAND LINE ERROR: " << e.what() << std::endl << opts); 
			exit(EXIT_FAILURE);
		}

		//----------------------------------------------------------------------
		// Initialize GLUT
		//----------------------------------------------------------------------

		InitGlut();

		//----------------------------------------------------------------------
		// Initialize OpenCL
		//----------------------------------------------------------------------

		SelectOpenCLDevices();
		InitOpenCLDevices();

		//----------------------------------------------------------------------
		// Run the application
		//----------------------------------------------------------------------

		return RunToy();
	} catch (cl::Error err) {
		OCLTOY_LOG("OpenCL ERROR: " << err.what() << "(" << OCLErrorString(err.err()) << ")");
		return EXIT_FAILURE;
	} catch (std::runtime_error err) {
		OCLTOY_LOG("RUNTIME ERROR: " << err.what());
		return EXIT_FAILURE;
	} catch (std::exception err) {
		OCLTOY_LOG("ERROR: " << err.what());
		return EXIT_FAILURE;
	}
}

//------------------------------------------------------------------------------
// OpenCL related code
//------------------------------------------------------------------------------

void OCLToy::SelectOpenCLDevices() {
	// Scan all platforms and devices available
	VECTOR_CLASS<cl::Platform> platforms;
	cl::Platform::get(&platforms);

	const std::string selectMethod = commandLineOpts["ocldevices"].as<std::string>();
	OCLTOY_LOG("OpenCL devices select method: " << selectMethod);

	size_t selectIndex = 0;
	boost::regex selRegex("[01]+");
	for (size_t i = 0; i < platforms.size(); ++i) {
		OCLTOY_LOG("OpenCL Platform " << i << ": " << platforms[i].getInfo<CL_PLATFORM_VENDOR>());
		OCLTOY_LOG("OpenCL Platform " << i << ": " << platforms[i].getInfo<CL_PLATFORM_VERSION>());

		// Get the list of devices available on the platform
		VECTOR_CLASS<cl::Device> devices;
		platforms[i].getDevices(CL_DEVICE_TYPE_ALL, &devices);

		for (size_t j = 0; j < devices.size(); ++j) {
			OCLTOY_LOG("  OpenCL device " << j << ": " << devices[j].getInfo<CL_DEVICE_NAME>());
			OCLTOY_LOG("    Version: " << devices[j].getInfo<CL_DEVICE_VERSION>());
			OCLTOY_LOG("    Type: " << OCLDeviceTypeString(devices[j].getInfo<CL_DEVICE_TYPE>()));
			OCLTOY_LOG("    Units: " << devices[j].getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>());
			OCLTOY_LOG("    Global memory: " << devices[j].getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>() / 1024 << "Kbytes");
			OCLTOY_LOG("    Local memory: " << devices[j].getInfo<CL_DEVICE_LOCAL_MEM_SIZE>() / 1024 << "Kbytes");
			OCLTOY_LOG("    Local memory type: " << OCLLocalMemoryTypeString(devices[j].getInfo<CL_DEVICE_LOCAL_MEM_TYPE>()));
			OCLTOY_LOG("    Constant memory: " << devices[j].getInfo<CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE>() / 1024 << "Kbytes");

			bool selected = false;
			if ((selectMethod == "ALL") ||
				((selectMethod == "ALL_GPUS") &&
					((devices[j].getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_GPU) || (devices[j].getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_ACCELERATOR))) ||
				((selectMethod == "ALL_CPUS") && (devices[j].getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_CPU)) ||
				((selectMethod == "FIRST_GPU") &&
					((devices[j].getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_GPU) || (devices[j].getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_ACCELERATOR)) &&
					(selectedDevices.size() == 0)) ||
				((selectMethod == "FIRST_CPU") && (devices[j].getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_CPU) && (selectedDevices.size() == 0)))
				selected = true;
			else if (boost::regex_match(selectMethod, selRegex)) {
				if (selectMethod.length() <= selectIndex)
					throw std::runtime_error("OpenCL select devices string (--ocldevices option) has the wrong length");
				if (selectMethod.at(selectIndex) == '1')
					selected = true;
				else if (selectMethod.at(selectIndex) != '0')
					throw std::runtime_error("Syntax error in OpenCL select devices string (--ocldevices option)");
			}

			if (selected) {
				if (selectedDevices.size() >= GetMaxDeviceCountSupported()) {
					OCLTOY_LOG("    NOT SELECTED because this toy supports only " << GetMaxDeviceCountSupported() << " device");
				} else {
					selectedDevices.push_back(devices[j]);
					OCLTOY_LOG("    SELECTED");
				}
			} else
				OCLTOY_LOG("    NOT SELECTED");

			++selectIndex;
		}
	}

	if (selectedDevices.size() == 0)
		throw std::runtime_error("Unable to find an OpenCL device");
}

void OCLToy::InitOpenCLDevices() {
	for (std::vector<cl::Device>::iterator dev = selectedDevices.begin(); dev < selectedDevices.end(); ++dev) {
		// Allocate a context with the selected device
		VECTOR_CLASS<cl::Device> devices;
		devices.push_back(*dev);

		cl::Context ctx(devices);
		deviceContexts.push_back(ctx);

		// Allocate the queue for this device
		cl::CommandQueue cmdQueue(ctx, *dev);
		deviceQueues.push_back(cmdQueue);
		
		deviceUsedMemory.push_back(0);
	}
}

void OCLToy::AllocOCLBufferRO(const unsigned int deviceIndex, cl::Buffer **buff,
		void *src, const size_t size, const std::string &desc) {
	cl::Device &oclDevice = selectedDevices[deviceIndex];

	// Check if the buffer is too big
	if (oclDevice.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>() < size) {
		std::stringstream ss;
		ss << "The " << desc << " buffer is too big for " << oclDevice.getInfo<CL_DEVICE_NAME>() << 
				" device (i.e. CL_DEVICE_MAX_MEM_ALLOC_SIZE=" <<
				oclDevice.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>() << ")";
		throw std::runtime_error(ss.str());
	}

	if (*buff) {
		// Check the size of the already allocated buffer
		if (size == (*buff)->getInfo<CL_MEM_SIZE>()) {
			// I can reuse the buffer; just update the content

			deviceQueues[deviceIndex].enqueueWriteBuffer(**buff, CL_TRUE, 0, size, src);
			return;
		} else {
			// Free the buffer
			deviceUsedMemory[deviceIndex] -= (*buff)->getInfo<CL_MEM_SIZE>();
			delete *buff;
		}
	}

	cl::Context &oclContext = deviceContexts[deviceIndex];

	OCLTOY_LOG( desc << " buffer size: " <<
			(size < 10000 ? size : (size / 1024)) << (size < 10000 ? "bytes" : "Kbytes"));
	*buff = new cl::Buffer(oclContext,
			CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
			size, src);
	deviceUsedMemory[deviceIndex] += (*buff)->getInfo<CL_MEM_SIZE>();
}

void OCLToy::AllocOCLBufferRW(const unsigned int deviceIndex, cl::Buffer **buff,
		const size_t size, const std::string &desc) {
	cl::Device &oclDevice = selectedDevices[deviceIndex];

	// Check if the buffer is too big
	if (oclDevice.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>() < size) {
		std::stringstream ss;
		ss << "The " << desc << " buffer is too big for " << oclDevice.getInfo<CL_DEVICE_NAME>() << 
				" device (i.e. CL_DEVICE_MAX_MEM_ALLOC_SIZE=" <<
				oclDevice.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>() << ")";
		throw std::runtime_error(ss.str());
	}

	if (*buff) {
		// Check the size of the already allocated buffer
		if (size == (*buff)->getInfo<CL_MEM_SIZE>()) {
			// I can reuse the buffer
			return;
		} else {
			// Free the buffer
			deviceUsedMemory[deviceIndex] -= (*buff)->getInfo<CL_MEM_SIZE>();
			delete *buff;
		}
	}

	cl::Context &oclContext = deviceContexts[deviceIndex];

	OCLTOY_LOG( desc << " buffer size: " <<
			(size < 10000 ? size : (size / 1024)) << (size < 10000 ? "bytes" : "Kbytes"));
	*buff = new cl::Buffer(oclContext,
			CL_MEM_READ_WRITE,
			size);
	deviceUsedMemory[deviceIndex] += (*buff)->getInfo<CL_MEM_SIZE>();
}

void OCLToy::AllocOCLBufferWO(const unsigned int deviceIndex, cl::Buffer **buff,
		const size_t size, const std::string &desc) {
	cl::Device &oclDevice = selectedDevices[deviceIndex];

	// Check if the buffer is too big
	if (oclDevice.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>() < size) {
		std::stringstream ss;
		ss << "The " << desc << " buffer is too big for " << oclDevice.getInfo<CL_DEVICE_NAME>() << 
				" device (i.e. CL_DEVICE_MAX_MEM_ALLOC_SIZE=" <<
				oclDevice.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>() << ")";
		throw std::runtime_error(ss.str());
	}

	if (*buff) {
		// Check the size of the already allocated buffer
		if (size == (*buff)->getInfo<CL_MEM_SIZE>()) {
			// I can reuse the buffer
			return;
		} else {
			// Free the buffer
			deviceUsedMemory[deviceIndex] -= (*buff)->getInfo<CL_MEM_SIZE>();
			delete *buff;
		}
	}

	cl::Context &oclContext = deviceContexts[deviceIndex];

	OCLTOY_LOG( desc << " buffer size: " <<
			(size < 10000 ? size : (size / 1024)) << (size < 10000 ? "bytes" : "Kbytes"));
	*buff = new cl::Buffer(oclContext,
			CL_MEM_WRITE_ONLY,
			size);
	deviceUsedMemory[deviceIndex] += (*buff)->getInfo<CL_MEM_SIZE>();
}

void OCLToy::FreeOCLBuffer(const unsigned int deviceIndex, cl::Buffer **buff) {
	if (*buff) {
		deviceUsedMemory[deviceIndex] += (*buff)->getInfo<CL_MEM_SIZE>();

		delete *buff;
		*buff = NULL;
	}
}

//------------------------------------------------------------------------------
// GLUT related code
//------------------------------------------------------------------------------

void OCLToy::ReshapeCallBack(int newWidth, int newHeight) {
	// Check if width or height have really changed
	if ((newWidth != windowWidth) ||
			(newHeight != windowHeight)) {
		windowWidth = newWidth;
		windowHeight = newHeight;

		glViewport(0, 0, windowWidth, windowHeight);
		glLoadIdentity();
		glOrtho(0.f, windowWidth - 1.f,
				0.f, windowHeight - 1.f, -1.f, 1.f);

		glutPostRedisplay();
	}
}

void OCLToy::GlutReshapeFunc(int newWidth, int newHeight) {
	currentOCLToy->ReshapeCallBack(newWidth, newHeight);
}

void OCLToy::GlutDisplayFunc() {
	currentOCLToy->DisplayCallBack();
}

void OCLToy::GlutTimerFunc(int value) {
	currentOCLToy->TimerCallBack(value);
}

void OCLToy::GlutKeyFunc(unsigned char key, int x, int y) {
	currentOCLToy->KeyCallBack(key, x, y);
}

void OCLToy::GlutSpecialFunc(int key, int x, int y) {
	currentOCLToy->SpecialCallBack(key, x, y);
}

void OCLToy::GlutMouseFunc(int button, int state, int x, int y) {
	currentOCLToy->MouseCallBack(button, state, x, y);
}

void OCLToy::GlutMotionFunc(int x, int y) {
	currentOCLToy->MotionCallBack(x, y);
}

void OCLToy::GlutIdleFunc() {
	currentOCLToy->IdleCallBack();
}

void OCLToy::InitGlut() {
	glutInitWindowSize(windowWidth, windowHeight);
	// Center the window
	const int scrWidth = glutGet(GLUT_SCREEN_WIDTH);
	const int scrHeight = glutGet(GLUT_SCREEN_HEIGHT);
	if ((scrWidth + 50 < windowWidth) || (scrHeight + 50 < windowHeight))
		glutInitWindowPosition(0, 0);
	else
		glutInitWindowPosition((scrWidth - windowWidth) / 2, (scrHeight - windowHeight) / 2);

	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutCreateWindow(windowTitle.c_str());

	glutReshapeFunc(&OCLToy::GlutReshapeFunc);
	glutKeyboardFunc(&OCLToy::GlutKeyFunc);
	glutSpecialFunc(&OCLToy::GlutSpecialFunc);
	glutDisplayFunc(&OCLToy::GlutDisplayFunc);
	glutMouseFunc(&OCLToy::GlutMouseFunc);
	glutMotionFunc(&OCLToy::GlutMotionFunc);
	if (millisTimerFunc > 0)
		glutTimerFunc(millisTimerFunc, &OCLToy::GlutTimerFunc, 0);
	if (useIdleCallback)
		glutIdleFunc(&OCLToy::GlutIdleFunc);

	glMatrixMode(GL_PROJECTION);
	glViewport(0, 0, windowWidth, windowHeight);
	glLoadIdentity();
	glOrtho(0.f, windowWidth - 1.f,
			0.f, windowHeight - 1.f, -1.f, 1.f);
}
