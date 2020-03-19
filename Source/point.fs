#version 400 core
#define MAX_LIGHTS 16

struct Light
{
	vec4 position;
	vec4 direction;
	
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	
	float attenuation;	//quadratic component of light's attenuation equation
	float cutOff;		//cosine of the angle of the inner fallof cone (spot lights)
	float outerCutOff;	//cos of the outer angle
	int type;			//determines the type of the light - directional, point or spot
	
	int shadowMapNr;
	float far;				//used for lights that use Ominidirectional Shadow Mapping (point lights)
	mat4 lightSpaceMatrix;	//used for lights that use 2D shadowmaps (directional and spot lights)
};

struct Fragment
{
	vec3 position;
	vec3 normal;
	vec3 albedo;
	float specular;
};

//out
out vec4 fragColor;

//uniform
uniform int lightIndex;
uniform vec2 resolution;
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform samplerCubeArray shadowCubemaps;
layout (std140) uniform Lights
{
	int lightCount;
	vec4 camPos;
	Light lights[MAX_LIGHTS];
};

////////////////////////////////
////////////////////////////////
////////////////////////////////

float CalcShadow3D(Light light, vec3 fragPosition)
{
	vec3 lightToFrag = fragPosition - light.position.xyz;
	float mapDepth = texture(shadowCubemaps, vec4(lightToFrag, light.shadowMapNr)).r;
	float realDepth = length(lightToFrag) / light.far;

	return ((realDepth > mapDepth + 0.01) ? (1.0) : (0.0));
}

vec3 CalcLight(Light light, Fragment frag)
{
	vec3 lightDir = normalize(light.position.rgb - frag.position);;

	//////////////
	
	vec3 ambient = light.ambient.rgb * frag.albedo;
	
	float diff = max(dot(frag.normal, lightDir), 0.0);
	vec3 diffuse = light.diffuse.rgb * frag.albedo * diff;
	
	vec3 camDir = normalize(camPos.xyz - frag.position);
	vec3 halfwayDir = normalize(camDir + lightDir);
	float spec = pow(max(dot(frag.normal, halfwayDir), 0.0), 64);
	vec3 specular = light.specular.rgb * frag.specular * spec;
	
	float attenuation = 1.0;
	if (light.attenuation != 0.0)
	{
		float dist = distance(frag.position, light.position.xyz);
		attenuation = 1.0 / (dist * dist * light.attenuation);
	}
	
	float shadow = 0.0;
	shadow = CalcShadow3D(light, frag.position);
	
	if (shadow == 1.0)
		return ambient * attenuation;
	
	return ((diffuse + specular) + ambient) * attenuation;
}

void main()
{
	vec2 texCoord = gl_FragCoord.xy / resolution;
	
	Fragment frag;
	frag.position = texture(gPosition, texCoord).rgb;
	frag.normal = texture(gNormal, texCoord).rgb;
	vec4 albedoSpec = texture(gAlbedoSpec, texCoord);
	frag.albedo = albedoSpec.rgb;
	frag.specular = albedoSpec.a;
	
	fragColor = vec4(CalcLight(lights[lightIndex], frag), 1.0);
}