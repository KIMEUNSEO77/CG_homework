// vertex shader
#version 330 core

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec3 vNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;   // 객체의 위치값을 프래그먼트 쉐이더로 보냄
out vec3 Normal;    // 법선 벡터를 프래그먼트 쉐이더로 보냄

void main() {
    FragPos = vec3(model * vec4(vPos, 1.0)); // 객체에 대한 조명 계산은 프래그먼트 쉐이더에서 함
                                             // 따라서, 월드공간에 있는 버텍스 값을 넘겨줘야 함
    Normal = mat3(transpose(inverse(model))) * vNormal;        // 법선 벡터도 프래그먼트 쉐이더로 넘겨줌
    gl_Position = projection * view * model * vec4(vPos, 1.0);
}