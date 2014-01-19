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

var MaterialType = {
	"MATTE" : 0, "MIRROR" : 1, "GLASS" : 2, "MATTETRANSLUCENT" : 3,
	"GLOSSY" : 4, "GLOSSYTRANSLUCENT" : 5
};

function Sphere() {
	this.rad = 1.0;
	
	this.p = vec3.create();
	this.e = vec3.create();
	
	this.material = MaterialType.MATTE;
	this.c = vec3.create([1.0, 1.0, 1.0]);
}

Sphere.prototype.parseSphere = function(sphereDefinition) {
	var sphereDefinitionArgs = sphereDefinition.split(" ");
	this.rad = parseFloat(sphereDefinitionArgs[1]);
	this.p = vec3.create([parseFloat(sphereDefinitionArgs[2]), parseFloat(sphereDefinitionArgs[3]), parseFloat(sphereDefinitionArgs[4])]);
	this.e = vec3.create([parseFloat(sphereDefinitionArgs[5]), parseFloat(sphereDefinitionArgs[6]), parseFloat(sphereDefinitionArgs[7])]);

	switch (parseInt(sphereDefinitionArgs[8])) {
		case MaterialType.MATTE:
			this.setMatte([parseFloat(sphereDefinitionArgs[9]), parseFloat(sphereDefinitionArgs[10]), parseFloat(sphereDefinitionArgs[11])]);
			break;
		case MaterialType.MIRROR:
			this.setMirror([parseFloat(sphereDefinitionArgs[9]), parseFloat(sphereDefinitionArgs[10]), parseFloat(sphereDefinitionArgs[11])]);
			break;
		case MaterialType.GLASS:
			this.setGlass([parseFloat(sphereDefinitionArgs[9]), parseFloat(sphereDefinitionArgs[10]), parseFloat(sphereDefinitionArgs[11])],
				parseFloat(sphereDefinitionArgs[12]), parseFloat(sphereDefinitionArgs[13]), parseFloat(sphereDefinitionArgs[14]));
			break;
		case MaterialType.MATTETRANSLUCENT:
			this.setMatteTranslucent([parseFloat(sphereDefinitionArgs[9]), parseFloat(sphereDefinitionArgs[10]), parseFloat(sphereDefinitionArgs[11])],
				parseFloat(sphereDefinitionArgs[12]), parseFloat(sphereDefinitionArgs[13]), parseFloat(sphereDefinitionArgs[14]));
			break;
		case MaterialType.GLOSSY:
			this.setGlossy([parseFloat(sphereDefinitionArgs[9]), parseFloat(sphereDefinitionArgs[10]), parseFloat(sphereDefinitionArgs[11])],
				parseFloat(sphereDefinitionArgs[12]));
			break;
		case MaterialType.GLOSSYTRANSLUCENT:
			this.setGlossyTranslucent([parseFloat(sphereDefinitionArgs[9]), parseFloat(sphereDefinitionArgs[10]), parseFloat(sphereDefinitionArgs[11])],
				parseFloat(sphereDefinitionArgs[12]), parseFloat(sphereDefinitionArgs[13]), parseFloat(sphereDefinitionArgs[14]),
				parseFloat(sphereDefinitionArgs[15]));
			break;
		default:
			alert("Unknown material in Sphere.parseSphere()");
	}
};

Sphere.prototype.setMatte = function(c) {
  this.material = MaterialType.MATTE;
  this.c = c;
};

Sphere.prototype.setMirror = function(c) {
  this.material = MaterialType.MIRROR;
  this.c = c;
};

Sphere.prototype.setGlass = function(c, ior, sigmas, sigmaa) {
  this.material = MaterialType.GLASS;
  this.c = c;
  this.ior = ior;
  this.sigmas = sigmas;
  this.sigmaa = sigmaa;
};

Sphere.prototype.setMatteTranslucent = function(c, transp, sigmas, sigmaa) {
  this.material = MaterialType.MATTETRANSLUCENT;
  this.c = c;
  this.transparency = transp;
  this.sigmas = sigmas;
  this.sigmaa = sigmaa;
};

Sphere.prototype.setGlossy = function(c, exp) {
  this.material = MaterialType.GLOSSY;
  this.c = c;
  this.exponent = exp;
};

Sphere.prototype.setGlossyTranslucent = function(c, exp, transp, sigmas, sigmaa) {
  this.material = MaterialType.GLOSSYTRANSLUCENT;
  this.c = c;
  this.exponent = exp;
  this.transparency = transp;
  this.sigmas = sigmas;
  this.sigmaa = sigmaa;
};

Sphere.getSizeInBytes = function() {
  return 15 * 4;
};

Sphere.prototype.setBuffer = function(buffer, offset) {
	var fbuffer = new Float32Array(buffer);
	var ibuffer = new Int32Array(buffer);

	fbuffer[offset] = this.rad;
	fbuffer[offset + 1] = this.p[0];
	fbuffer[offset + 2] = this.p[1];
	fbuffer[offset + 3] = this.p[2];
	fbuffer[offset + 4] = this.e[0];
	fbuffer[offset + 5] = this.e[1];
	fbuffer[offset + 6] = this.e[2];
	switch (this.material) {
		case MaterialType.MATTE:
			ibuffer[offset + 7] = 0;
			fbuffer[offset + 8] = this.c[0];
			fbuffer[offset + 9] = this.c[1];
			fbuffer[offset + 10] = this.c[2];
			fbuffer[offset + 11] = 0;
			fbuffer[offset + 12] = 0;
			fbuffer[offset + 13] = 0;
			fbuffer[offset + 14] = 0;
			break;
		case MaterialType.MIRROR:
			ibuffer[offset + 7] = 1;
			fbuffer[offset + 8] = this.c[0];
			fbuffer[offset + 9] = this.c[1];
			fbuffer[offset + 10] = this.c[2];
			fbuffer[offset + 11] = 0;
			fbuffer[offset + 12] = 0;
			fbuffer[offset + 13] = 0;
			fbuffer[offset + 14] = 0;
			break;
		case MaterialType.GLASS:
			ibuffer[offset + 7] = 2;
			fbuffer[offset + 8] = this.c[0];
			fbuffer[offset + 9] = this.c[1];
			fbuffer[offset + 10] = this.c[2];
			fbuffer[offset + 11] = this.ior;
			fbuffer[offset + 12] = this.sigmas;
			fbuffer[offset + 13] = this.sigmaa;
			fbuffer[offset + 14] = 0;
			break;
		case MaterialType.MATTETRANSLUCENT:
			ibuffer[offset + 7] = 3;
			fbuffer[offset + 8] = this.c[0];
			fbuffer[offset + 9] = this.c[1];
			fbuffer[offset + 10] = this.c[2];
			fbuffer[offset + 11] = this.transparency;
			fbuffer[offset + 12] = this.sigmas;
			fbuffer[offset + 13] = this.sigmaa;
			fbuffer[offset + 14] = 0;
			break;
		case MaterialType.GLOSSY:
			ibuffer[offset + 7] = 4;
			fbuffer[offset + 8] = this.c[0];
			fbuffer[offset + 9] = this.c[1];
			fbuffer[offset + 10] = this.c[2];
			fbuffer[offset + 11] = this.exponent;
			fbuffer[offset + 12] = 0;
			fbuffer[offset + 13] = 0;
			fbuffer[offset + 14] = 0;
			break;
		case MaterialType.GLOSSYTRANSLUCENT:
			ibuffer[offset + 7] = 5;
			fbuffer[offset + 8] = this.c[0];
			fbuffer[offset + 9] = this.c[1];
			fbuffer[offset + 10] = this.c[2];
			fbuffer[offset + 11] = this.exponent;
			fbuffer[offset + 12] = this.transparency;
			fbuffer[offset + 13] = this.sigmas;
			fbuffer[offset + 14] = this.sigmaa;
			break;
		default:
			alert("Unknown material in Sphere.setBuffer()");
	}
};
