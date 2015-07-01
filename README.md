Scene Reconstruction
====================
Generates a 3D model reconstruction of a scene from stereo video.
The process is based on the [KinectFusion][1] paper by Microsoft Research with the additional first step of computing disparity from the stereo footage.

Pipeline
========
The process is broken into stages:
 1. Extracting depth from the scene by computing stereo disparity  
 2. Converting the pixel disparity values into real word depth values
 3. Tracking the pose of the camera between frames
 4. Integrating the data from the frame into the volumetric 3D model
 5. Rendering the volumetric model via ray casting

To do
=====
 * Tracking the pose of the camera (work in progress)
 * Integrating the data from the frame

License
=====
	Copyright 2015 Jaween Ediriweera

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

		http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.




[1]: http://research.microsoft.com/en-us/projects/surfacerecon/
