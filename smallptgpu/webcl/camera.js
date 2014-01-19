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

function Camera() {
	// User defined values
	this.orig = vec3.create();
	this.target = vec3.create();
	
	// Calculated values
	this.dir = vec3.create();
	this.x = vec3.create();
	this.y = vec3.create();
}

Camera.prototype.update = function(width, height) {
	vec3.subtract(this.target, this.orig, this.dir);
	vec3.normalize(this.dir);

	var up = vec3.create([0.0, 1.0, 0.0]);
	var fov = (M_PI / 180.0) * 45.0;
	vec3.cross(this.dir, up, this.x);
	vec3.normalize(this.x);
	vec3.scale(this.x, width * fov / height, this.x);
	
	vec3.cross(this.x, this.dir, this.y);
	vec3.normalize(this.y);
	vec3.scale(this.y, fov, this.y);
};

Camera.prototype.getBuffer = function() {
	var buffer = new Float32Array(15);

	buffer[0] = this.orig[0];
	buffer[1] = this.orig[1];
	buffer[2] = this.orig[2];

	buffer[3] = this.target[0];
	buffer[4] = this.target[1];
	buffer[5] = this.target[2];

	buffer[6] = this.dir[0];
	buffer[7] = this.dir[1];
	buffer[8] = this.dir[2];

	buffer[9] = this.x[0];
	buffer[10] = this.x[1];
	buffer[11] = this.x[2];

	buffer[12] = this.y[0];
	buffer[13] = this.y[1];
	buffer[14] = this.y[2];

	return buffer;
};

Camera.prototype.getBufferSizeInBytes = function() {
	return 15 * 4;
};
