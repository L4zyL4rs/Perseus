#version 450

layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;

layout(push_constant) uniform constants {
  vec4 sunPos;
  vec4 camForward;
  vec4 camUp;
  vec4 FOV;  // Horizontal and vertical FOV
} sky;

void main() {
  // EVERYTHING SHOULD BE KEPT IN CAMERA COORDINATES
  // X IS FORWARD
  // Y IS RIGHT
  // Z IS UP
  
  // INPUT UV IS RIGHT, UP

  vec4 horizonColour = vec4(0.56, 0.7, 0.96, 1.0);
  vec4 zenithColour = vec4(0.192157f, 0.325490f, 0.564706f, 1.0);

  vec3 camRight = normalize(cross(sky.camForward.xyz, sky.camUp.xyz));
  vec3 camUp = -normalize(cross(camRight, sky.camForward.xyz));

  vec3 sunDir = vec3(dot(sky.sunPos.xyz, sky.camForward.xyz),
                     dot(sky.sunPos.xyz, camRight),
                     dot(sky.sunPos.xyz, camUp));
  vec4 sunPos = vec4(0, 0.7, 1, 0);
  vec4 sunCoreColor = vec4(1.0, 1.0, 1.0, 1.0);
  vec4 sunOuterColor = vec4(0.905882f, 0.956863f, 0.980392f, 1.0f);

  float tanHalfVertical = tan(radians(sky.FOV.y) * 0.5);
  float tanHalfHorizontal = tan(radians(sky.FOV.x) * 0.5);

  vec2 ndc = uv * 2.0 - 1.0;
  ndc.y = ndc.y;

  vec3 rayDir = normalize(vec3(
      1.0,
      ndc.x * tanHalfHorizontal,
      ndc.y * tanHalfVertical
  ));

  vec3 worldRay = normalize(sky.camForward.xyz + rayDir.y * camRight + rayDir.z * camUp);
  float sunAmount = dot(sunDir, rayDir);
  float sunDisk = smoothstep(0.999, 1.0, sunAmount);
  vec4 sunColor = mix(sunOuterColor, sunCoreColor, sunDisk);

  vec4 background = mix(horizonColour, zenithColour, smoothstep(0.0, 0.6, worldRay.z));

  outColor = mix(background, sunColor, sunDisk);
}
