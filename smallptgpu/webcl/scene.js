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

function Scene() {
	this.camera = new Camera();
	this.spheres = [];
	this.defaultMaxDepth = 6;
	this.defaultSigmaS = 0.0;
	this.defaultSigmaA = 0.0;
}

Scene.prototype.getCamera = function() {
	return this.camera;
};

Scene.prototype.getSpheres = function() {
	return this.spheres;
};

Scene.prototype.getSphereCount = function() {
	return this.spheres.length;
};

Scene.prototype.getSpheresBuffer = function() {
	buffer = new ArrayBuffer(Sphere.getSizeInBytes() * this.spheres.length);

	var size = Sphere.getSizeInBytes() / 4;
	for(var i = 0; i < this.spheres.length; i++) {
		var offset = size * i;
		this.spheres[i].setBuffer(buffer, offset);
	}

	return new Float32Array(buffer);
};

Scene.prototype.getSpheresBufferSizeInBytes = function() {
	return this.spheres.length * Sphere.getSizeInBytes();
};

Scene.prototype.parseScene = function(sceneStr) {
	var lines = lines = sceneStr.split(/\r\n|\r|\n/);

	// Parse camera
	var cameraArgs = lines[0].split(" ");
	this.camera.orig = vec3.create([parseFloat(cameraArgs[1]), parseFloat(cameraArgs[2]), parseFloat(cameraArgs[3])]);
	this.camera.target = vec3.create([parseFloat(cameraArgs[4]), parseFloat(cameraArgs[5]), parseFloat(cameraArgs[6])]);

	// Parse max. path depth
	var maxDepthArgs = lines[1].split(" ");
	this.defaultMaxDepth = parseInt(maxDepthArgs[1]);
	// Parse defualt sigma s
	var defaultSigmaSArgs = lines[2].split(" ");
	this.defaultSigmaS = parseFloat(defaultSigmaSArgs[1]);
	// Parse defualt sigma a
	var defaultSigmaAArgs = lines[3].split(" ");
	this.defaultSigmaA = parseFloat(defaultSigmaAArgs[1]);

	// Parse sphere count
	var sphereCountArgs = lines[4].split(" ");
	var sphereCount = parseInt(sphereCountArgs[1]);
	var currentLine = 5;
	this.spheres = [];
	for (var i = 0; i < sphereCount; ++i) {
		var sphere = new Sphere();
		sphere.parseSphere(lines[currentLine++]);
		this.spheres.push(sphere);
	}
};