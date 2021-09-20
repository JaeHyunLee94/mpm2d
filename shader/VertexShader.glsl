#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec2 vertexPosition_modelspace;

uniform mat4 viewMat,projMat;


void main(){
    gl_Position.xyz = (projMat*viewMat*vec4(vertexPosition_modelspace,0.0,1.0)).xyz;
    gl_Position.w = 1.0;

}

