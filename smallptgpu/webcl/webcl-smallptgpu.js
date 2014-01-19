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

// This code is based on the original Matias Piispanen's port of
// SmallPTGPU to WebCL

var M_PI = 3.14159265358979323846;
var maxint = 2147483647;

var selected = 0;
var selectedPlatform;
var canvas;
var canvasContext;

var cl;
var clQueue;
var clSrc;
var clProgram;
var clKernelsSmallPT;
var workGroupSizeSmallPT;
var clKernelsToneMapping;
var workGroupSizeToneMapping;
var kernelIterations = 1;
var currentSample = 0;

var clSphereBuffer;
var clCameraBuffer;
var clPixelsBuffer;
var clColorBuffer;
var clSeedBuffer;

var pixelCount;
var htmlConsole;
var currentSphere = 0;
var sceneName = "cornell.scn";
var scene;

var pixels;
var canvasContent;
var pixel8View;
var pBuffer;

var clTime = 0;
var jsTime = 0;
var sampleSec = 0;
var prevTime = Date.now();

var running = true;

function resetStats() {
	clTime = 0;
	jsTime = 0;
	sampleSec = 0;
	prevTime = Date.now();
}

function xhrLoad(uri) {
	var xhr = new XMLHttpRequest();
	xhr.open("GET", uri, false);
	xhr.send(null);
	if (xhr.status == 200) {
		//console.log(xhr.responseText);
		return xhr.responseText;
	} else {
		console.log("XMLHttpRequest error!");
		return null;
	}
};

requestAnimationFrame = window.requestAnimationFrame || window.mozRequestAnimationFrame ||  
	window.webkitRequestAnimationFrame || window.msRequestAnimationFrame;  

var start = window.mozAnimationStartTime;  // Only supported in FF. Other browsers can use something like Date.now(). 

function webCLSmallPT() {
	htmlConsole = document.getElementById("console");
	canvas = document.getElementById("canvas");
	canvasContext = canvas.getContext("2d");
  
	var sceneSrc = xhrLoad("scenes/" + sceneName);

	scene = new Scene();
	scene.parseScene(sceneSrc);
	scene.camera.update(canvas.width, canvas.height);

	resetStats();

	if (setupWebCL()) {
		canvasContent = canvasContext.createImageData(canvas.width, canvas.height);

		updateRendering();
		running = true;
		requestAnimationFrame(step, canvas);  
	} else
		running = false;

	updateStartStopButton();
} 

function step(timestamp) {  
	if(running == true) {
		jsTime = jsTime * 0.95 + 0.05 * (Date.now() - prevTime);

		htmlConsole.innerHTML = "WebCL: " + clTime.toFixed(2) + "ms" +
			"<br>JS: " + jsTime.toFixed(2) + "ms";
		updateRendering();

		prevTime = Date.now();
		requestAnimationFrame(step, canvas);  
	}
}  

function updateScene() {
	kernelIterations = 1;
	currentSample = 0;
	resetStats();

	clQueue.enqueueWriteBuffer(clSphereBuffer, true, 0, scene.getSpheresBufferSizeInBytes(),
		scene.getSpheresBuffer(), []);
}

function updateCamera() {
	kernelIterations = 1;
	currentSample = 0;
	resetStats();
	
	scene.getCamera().update(canvas.width, canvas.height);

	clQueue.enqueueWriteBuffer(clCameraBuffer, true, 0, scene.getCamera().getBufferSizeInBytes(),
		scene.getCamera().getBuffer(), []);
}

function drawPixels() {
	pixel8View = new Uint8ClampedArray(pBuffer);
	canvasContent.data.set(pixel8View);
	canvasContext.putImageData(canvasContent, 0, 0);
}

function updateStartStopButton() {
	if(running) {
		requestAnimationFrame(step, canvas);
		document.getElementById("stop").innerHTML = "Stop";
	} else {
		document.getElementById("stop").innerHTML = "Start";
	}
}

function startStopChanged() {
	running = (!running);

	updateStartStopButton();
}

function resolutionChanged(resolution) {
	running = false;
	
	if(resolution == 0) {
		canvas.width = 320;
		canvas.height = 240;
	} else if(resolution == 1) {
		canvas.width = 640;
		canvas.height = 480;
	} else if(resolution == 2) {
		canvas.width = 800;
		canvas.height = 600;
	}

	freeBuffers();
	webCLSmallPT();
	updateCamera();
}

function sceneChanged(name) {
	running = false;
	currentSphere = 0;

	sceneName = name;

	freeBuffers();
	webCLSmallPT();
	updateCamera();
}

function keyFunc(event) {
	var key = event.keyCode;
	var MOVE_STEP = 10.0;
	var ROTATE_STEP = 2.0 * M_PI / 180.0;

	var up = 38;
	var down = 40;
	var left = 39;
	var right = 37;
	
	var two = 50;
	var twoKeypad = 98;
	var three = 51;
	var threeKeypad = 99;
	var four = 52;
	var fourKeypad = 100;
	var six = 54;
	var sixKeypad = 102;
	var eight = 56;
	var eightKeypad = 104;
	var nine = 57;
	var nineKeypad = 105;
	
	var plus = 77;
	var minus = 78;
	
	var w = 87;
	var a = 65;
	var s = 83;
	var d = 68;
	var r = 82;
	var f = 70;
	
	var pgup = 33;
	var pgdown = 34;
	
	switch(key) {
		case up:
			var t = vec3.create(scene.getCamera().target);
			vec3.subtract(t, scene.getCamera().orig, t);
			t[1] = t[1] * Math.cos(-ROTATE_STEP) + t[2] * Math.sin(-ROTATE_STEP);
			t[2] = -t[1] * Math.sin(-ROTATE_STEP) + t[2] * Math.cos(-ROTATE_STEP);
			vec3.add(t, scene.getCamera().orig, t);
			scene.getCamera().target = t;
			updateCamera();
			break;
		case down:
			var t = vec3.create(scene.getCamera().target);
			vec3.subtract(t, scene.getCamera().orig, t);
			t[1] = t[1] * Math.cos(ROTATE_STEP) + t[2] * Math.sin(ROTATE_STEP);
			t[2] = -t[1] * Math.sin(ROTATE_STEP) + t[2] * Math.cos(ROTATE_STEP);
			vec3.add(t, scene.getCamera().orig, t);
			scene.getCamera().target = t;
			updateCamera();
			break;
		case left:
			var t = vec3.create(scene.getCamera().target);
			vec3.subtract(t, scene.getCamera().orig, t);
			t[0] = t[0] * Math.cos(-ROTATE_STEP) - t[2] * Math.sin(ROTATE_STEP);
			t[2] = t[0] * Math.sin(-ROTATE_STEP) + t[2] * Math.cos(ROTATE_STEP);
			vec3.add(t, scene.getCamera().orig, t);
			scene.getCamera().target = t;
			updateCamera();
			break;
		case right:
			var t = vec3.create(scene.getCamera().target);
			vec3.subtract(t, scene.getCamera().orig, t);
			t[0] = t[0] * Math.cos(ROTATE_STEP) - t[2] * Math.sin(-ROTATE_STEP);
			t[2] = t[0] * Math.sin(ROTATE_STEP) + t[2] * Math.cos(-ROTATE_STEP);
			vec3.add(t, scene.getCamera().orig, t);
			scene.getCamera().target = t;
			updateCamera();
			break;
		case pgup:
			scene.getCamera().target[1] += MOVE_STEP;
			updateCamera();
			break;
		case pgdown:
			scene.getCamera().target[1] -= MOVE_STEP;
			updateCamera();
			break;
		case w:
			var dir = vec3.create(scene.getCamera().dir);
			vec3.scale(dir, MOVE_STEP);
			vec3.add(scene.getCamera().orig, dir, scene.getCamera().orig);
			vec3.add(scene.getCamera().target, dir, scene.getCamera().target);
			updateCamera();
			break;
		case a:
			var dir = vec3.create(scene.getCamera().x);
			vec3.normalize(dir);
			vec3.scale(dir, -MOVE_STEP);
			vec3.add(scene.getCamera().orig, dir, scene.getCamera().orig);
			vec3.add(scene.getCamera().target, dir, scene.getCamera().target);
			updateCamera();
			break;
		case s:
			var dir = vec3.create(scene.getCamera().dir);
			vec3.scale(dir, -MOVE_STEP);
			vec3.add(scene.getCamera().orig, dir, scene.getCamera().orig);
			vec3.add(scene.getCamera().target, dir, scene.getCamera().target);
			updateCamera();
			break;
			;
		case d:
			var dir = vec3.create(scene.getCamera().x);
			vec3.normalize(dir);
			vec3.scale(dir, MOVE_STEP);
			vec3.add(scene.getCamera().orig, dir, scene.getCamera().orig);
			vec3.add(scene.getCamera().target, dir, scene.getCamera().target);
			updateCamera();
			break;
		case r:
			scene.getCamera().orig[1] += MOVE_STEP;
			scene.getCamera().target[1] += MOVE_STEP;
			updateCamera();
			break;
		case f:
			scene.getCamera().orig[1] -= MOVE_STEP;
			scene.getCamera().target[1] -= MOVE_STEP;
			updateCamera();
			break;
		case four:
		case fourKeypad:
			var sArray = scene.getSpheres();
			sArray[currentSphere].p[0] -= 0.5 * MOVE_STEP;
			updateScene();
			break;
		case six:
		case sixKeypad:
			var sArray = scene.getSpheres();
			sArray[currentSphere].p[0] += 0.5 * MOVE_STEP;
			updateScene(); 
			break;
		case eight:
		case eightKeypad:
			var sArray = scene.getSpheres();
			sArray[currentSphere].p[2] -= 0.5 * MOVE_STEP;
			updateScene(); 
			break;
		case two:
		case twoKeypad:
			var sArray = scene.getSpheres();
			sArray[currentSphere].p[2] += 0.5 * MOVE_STEP;
			updateScene(); 
			break;
		case nine:
		case nineKeypad:
			var sArray = scene.getSpheres();
			sArray[currentSphere].p[1] += 0.5 * MOVE_STEP;
			updateScene(); 
			break;
		case three:
		case threeKeypad:
			var sArray = scene.getSpheres();
			sArray[currentSphere].p[1] -= 0.5 * MOVE_STEP;
			updateScene(); 
			break;
		case plus:
			currentSphere = (currentSphere + 1) % scene.spheres.length;
			break;
		case minus:
			currentSphere = (currentSphere + (scene.spheres.length - 1)) % scene.spheres.length;
			break;
		default:
			break;
	}
}

function deviceChanged(device) {
	running = false;

	kernelIterations = 1;
	currentSample = 0;
	freeBuffers();

	selected = device;
	
	webCLSmallPT();
}

function freeBuffers() {
	clSphereBuffer.releaseCLResources();
	clCameraBuffer.releaseCLResources();
	clPixelsBuffer.releaseCLResources();
	clColorBuffer.releaseCLResources();
	clSeedBuffer.releaseCLResources();

	clQueue.releaseCLResources();
	clProgram.releaseCLResources();
	clKernelsSmallPT.releaseCLResources();
	clKernelsToneMapping.releaseCLResources();
	cl.releaseCLResources();
}

function allocateBuffers() {
	clSphereBuffer = cl.createBuffer(WebCL.CL_MEM_READ_ONLY, scene.getSpheresBufferSizeInBytes());
	clQueue.enqueueWriteBuffer(clSphereBuffer, true, 0, scene.getSpheresBufferSizeInBytes(), scene.getSpheresBuffer(), []);
	
	clCameraBuffer = cl.createBuffer(WebCL.CL_MEM_READ_ONLY, scene.getCamera().getBufferSizeInBytes());
	clQueue.enqueueWriteBuffer(clCameraBuffer, true, 0, scene.getCamera().getBufferSizeInBytes(), scene.getCamera().getBuffer(), []);

	pixelCount = canvas.width * canvas.height;
	pBuffer = new ArrayBuffer(4 * pixelCount);
	pixelArray = new Int32Array(pBuffer);
	pixel8View = new Uint8ClampedArray(pixelCount * 4);

	for(var i = 0; i < pixelCount * 4; i++) {
		pixel8View[i] = i % 255;
	}
	
	var seeds = new Uint32Array(pixelCount * 2);
	for(var i = 0; i < pixelCount * 2; i++) {
		seeds[i] = Math.random() * maxint;
		
		if(seeds[i] < 2) {
			seeds[i] = 2;
		}
	}

	clColorBuffer = cl.createBuffer(WebCL.CL_MEM_READ_WRITE, 3 * 4 * pixelCount);
	clPixelsBuffer = cl.createBuffer(WebCL.CL_MEM_WRITE_ONLY, 4 * pixelCount);
	clSeedBuffer = cl.createBuffer(WebCL.CL_MEM_READ_WRITE, 4 * pixelCount * 2);	
	clQueue.enqueueWriteBuffer(clSeedBuffer, true, 0, 4 * pixelCount * 2, seeds, []);
}

function clDeviceQuery() {
	var deviceList = [];
	var platforms = (window.WebCL && WebCL.getPlatforms()) || [];
	for (var p = 0, i = 0; p < platforms.length; p++) {
		var plat = platforms[p];
		var devices = plat.getDevices(WebCL.CL_DEVICE_TYPE_ALL);
		for (var d = 0; d < devices.length; d++, i++) {
			if (devices[d].getDeviceInfo(WebCL.CL_DEVICE_AVAILABLE) === true) {
				var availableDevice = {
					'device' : devices[d], 
					'type' : devices[d].getDeviceInfo(WebCL.CL_DEVICE_TYPE),
					'name' : devices[d].getDeviceInfo(WebCL.CL_DEVICE_NAME),
					'version' : devices[d].getDeviceInfo(WebCL.CL_DEVICE_VERSION),
					'vendor' : plat.getPlatformInfo(WebCL.CL_PLATFORM_VENDOR),
					'platform' : plat
				};
				deviceList.push(availableDevice);
			}
		}
	}
	console.log(deviceList);
	return deviceList;
}

function setupWebCL() {
	var deviceList = clDeviceQuery();

	if (deviceList.length === 0) {
		alert("Unfortunately your browser/system doesn't support WebCL.");
		return false;
	}

	try {
		var htmlDeviceList = "";
		for(var i in deviceList) {
			htmlDeviceList += "<option value=" + i + ">" + deviceList[i].vendor + ": " + deviceList[i].name + "</option>\n";
		}

		var deviceselect = document.getElementById("devices");
		deviceselect.innerHTML = htmlDeviceList;
		deviceselect.selectedIndex = selected;

		var selectedDevice = deviceList[selected].device;
		var selectedPlatform = deviceList[selected].platform;
		cl = WebCL.createContext([WebCL.CL_CONTEXT_PLATFORM, selectedPlatform], [selectedDevice]);
		clQueue = cl.createCommandQueue(selectedDevice, null);
	} catch(err) {
		alert("Error initializing WebCL: " + err);
		console.log(err);
		return false;
	}

	allocateBuffers();

	try {
		clSrc = xhrLoad("preprocessed_rendering_kernel.cl");
		//console.log(clSrc);
		clProgram = cl.createProgramWithSource(clSrc);

		var opts ="-DPARAM_MAX_DEPTH=" + scene.defaultMaxDepth +
			" -DPARAM_DEFAULT_SIGMA_S=" + scene.defaultSigmaS +
			" -DPARAM_DEFAULT_SIGMA_A=" + scene.defaultSigmaA;
		console.log("Kernel options: " + opts);

		clProgram.buildProgram([selectedDevice],opts);		
		var buildLog = clProgram.getProgramBuildInfo(selectedDevice, WebCL.CL_PROGRAM_BUILD_LOG);
		console.log("Kernel build log: ");
		console.log(buildLog);
	} catch(err) {
		alert("Failed to build WebCL program: " + err + "\nError " + 
			clProgram.getProgramBuildInfo(selectedDevice, WebCL.CL_PROGRAM_BUILD_STATUS) + ":  " + 
			clProgram.getProgramBuildInfo(selectedDevice, WebCL.CL_PROGRAM_BUILD_LOG));
		console.log(err);
		return false;
	}
	
	clKernelsSmallPT = clProgram.createKernel("SmallPTGPU");
	workGroupSizeSmallPT = clKernelsSmallPT.getKernelWorkGroupInfo(selectedDevice, WebCL.CL_KERNEL_WORK_GROUP_SIZE);

	clKernelsToneMapping = clProgram.createKernel("WebCLToneMapping");
	workGroupSizeToneMapping = clKernelsToneMapping.getKernelWorkGroupInfo(selectedDevice, WebCL.CL_KERNEL_WORK_GROUP_SIZE);

	return true;
}

function executeSmallPTKernel() {
	var globalThreadsSmallPT = canvas.width * canvas.height;	
	if(globalThreadsSmallPT % workGroupSizeSmallPT !== 0) {
		globalThreadsSmallPT = (Math.floor(globalThreads / workGroupSizeSmallPT) + 1) * workGroupSizeSmallPT;
	}

	clKernelsSmallPT.setKernelArg(0, clColorBuffer);
	clKernelsSmallPT.setKernelArg(1, clSeedBuffer);
	clKernelsSmallPT.setKernelArg(2, clCameraBuffer);
	clKernelsSmallPT.setKernelArg(3, scene.spheres.length, WebCL.types.UINT);
	clKernelsSmallPT.setKernelArg(4, clSphereBuffer);
	clKernelsSmallPT.setKernelArg(5, canvas.width, WebCL.types.INT);
	clKernelsSmallPT.setKernelArg(6, canvas.height, WebCL.types.INT);
	clKernelsSmallPT.setKernelArg(7, currentSample++, WebCL.types.INT);

	try {
		clQueue.enqueueNDRangeKernel(clKernelsSmallPT, 1, [], [globalThreadsSmallPT], [workGroupSizeSmallPT], []);
	} catch(err) {
		htmlConsole.innerHTML += "<br>SmallPT kernel execution error: <code>" + err + "</code><br>";
	}
}

function executeToneMappingKernel() {
	var globalThreadsToneMapping = canvas.width * canvas.height;	
	if(globalThreadsToneMapping % workGroupSizeToneMapping !== 0) {
		globalThreadsToneMapping = (Math.floor(globalThreadsToneMapping / workGroupSizeToneMapping) + 1) * workGroupSizeToneMapping;
	}

	clKernelsToneMapping.setKernelArg(0, clColorBuffer);
	clKernelsToneMapping.setKernelArg(1, clPixelsBuffer);
	clKernelsToneMapping.setKernelArg(2, canvas.width, WebCL.types.INT);
	clKernelsToneMapping.setKernelArg(3, canvas.height, WebCL.types.INT);

	try {
		clQueue.enqueueNDRangeKernel(clKernelsToneMapping, 1, [], [globalThreadsToneMapping], [workGroupSizeToneMapping], []);
	} catch(err) {
		htmlConsole.innerHTML += "<br>Tone Mpping kernel execution error: <code>" + err + "</code><br>";
	}
}

function updateRendering() {
	var t0 = Date.now();
	for (var i = 0; i < kernelIterations; ++i)
		executeSmallPTKernel();

	executeToneMappingKernel();
	clQueue.enqueueReadBuffer(clPixelsBuffer, false, 0, 4 * pixelCount, pixelArray, []);
	clQueue.finish();
	var t1 = Date.now();

	var elapsedTime = t1 -t0;
	clTime = clTime * 0.95 + 0.05 * elapsedTime;
	if (elapsedTime < 30)
		kernelIterations++;
	else {
		kernelIterations = Math.max(--kernelIterations, 1);
	}

	var samples = kernelIterations * canvas.height * canvas.width;
	sampleSec =  sampleSec * 0.95 + 0.05 * samples / elapsedTime;
	
	htmlConsole.innerHTML += "<br>Pass: " + currentSample +
		"<br>Sample/sec: " + (sampleSec / 1000.0).toFixed(2) + "M" +
		"<br>Iterations per step: " + kernelIterations +
		"<br>Current sphere: " + currentSphere + "\n";
	
	drawPixels();
} 
