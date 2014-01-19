#ifndef ANIMATION_H
#define ANIMATION_H

/*****************************************************************************
 *                                 jugCLer                                   *
 *             realtime raytracing experiment: animation headers             *
 *                                                                           *
 *  Copyright (C) 2013  Holger Bettag               (hobold@vectorizer.org)  *
 *                                                                           *
 *  This program is free software; you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation; either version 2 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License along  *
 *  with this program; if not, write to the Free Software Foundation, Inc.,  *
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.              *
 *****************************************************************************/

#include "scene.h"

extern double sceneTimeOffset;

// compute scene state for current time
void animatePositions(Scene* scene, const bool updateCamera);

// initialize scene state
void setupAnim(Scene* scene, int imgWidth, int imgHeight);

#endif   // ANIMATION_H
