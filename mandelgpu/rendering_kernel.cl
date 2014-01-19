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

__kernel void mandelGPU(
		__global int *pixels,
		const int width,
		const int height,
		const float scale,
		const float offsetX,
		const float offsetY,
		const int maxIterations
		) {
	const int gid = get_global_id(0);
	const int gid4 = 4 * gid;
	const int maxSize = max(width, height);
	const float kx = (scale / 2.f) * width;
	const float ky = (scale / 2.f) * height;

	int t;
	unsigned int iter[4];
	for (t = 0; t < 4; ++t) {
		const int tid = gid4 + t;

		const int screenX = tid % width;
		const int screenY = tid / width;

		// Check if we have something to do
		if (screenY >= height)
			return;

		const float x0 = ((screenX * scale) - kx) / maxSize + offsetX;
		const float y0 = ((screenY * scale) - ky) / maxSize + offsetY;

		float x = x0;
		float y = y0;
		float x2 = x * x;
		float y2 = y * y;
		for (iter[t] = 0; (x2 + y2 <= 4.f) && (iter[t] < maxIterations); ++iter[t]) {
			y = 2 * x * y + y0;
			x = x2 - y2 + x0;

			x2 = x * x;
			y2 = y * y;
		}

		if (iter[t] == maxIterations)
			iter[t] = 0;
		else {
			iter[t] = iter[t] % 512;
			if (iter[t] > 255)
				iter[t] = 511 - iter[t];
		}
	}

	pixels[gid] = iter[0] |
			(iter[1] << 8) |
			(iter[2] << 16) |
			(iter[3] << 24);
}
