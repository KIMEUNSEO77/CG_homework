// fragment shader
#version 330 core
in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform vec3 objectColor;   // 응용 프로그램에서 설정한 객체 색상
uniform vec3 lightColor;    // 응용 프로그램에서 설정한 광원(조명) 색상
uniform vec3 lightPos;      // 광원의 위치
uniform vec3 viewPos;	   // 카메라(뷰어)의 위치

uniform bool lightOn;      // 조명 켜기/끄기 여부

void main() {
    float ambientLight = 0.5; // 주변광 강도
    vec3 ambient = ambientLight * lightColor; // 주변광 계산

    vec3 normalVector = normalize(Normal); // 법선 벡터 정규화
    vec3 lightDir = normalize(lightPos - FragPos); // 광원 방향 벡터 계산
    float diffuseLight = max(dot(normalVector, lightDir), 0.0); // 확산광 강도 계산
    vec3 diffuse = diffuseLight * lightColor; // 확산광 계산

    int shininess = 128; // 광택 계수
    vec3 viewDir = normalize(viewPos-FragPos); // 뷰어 방향 벡터 계산 (카메라가 원점에 있다고 가정)
    vec3 reflectDir = reflect(-lightDir, normalVector); // 반사 벡터 계산
    float specularLight = pow(max(dot(viewDir, reflectDir), 0.0), shininess); // 반사광 강도 계산
    vec3 specular = specularLight * lightColor; // 반사광 계산

    vec3 result = (ambient + diffuse + specular) * objectColor; // 최종 색상 계산

    if (!lightOn) 
    {
        result = objectColor * ambient; // 조명이 꺼져있으면 객체 색상만 사용
    }
    FragColor = vec4(result, 1.0);
}