#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

// texture sampler
uniform bool horizontal;
uniform sampler2D image;
uniform float weight[30];
uniform float kernelSize;

void main()
{
	vec2 tex_offset=1.0/textureSize(image,0);
	vec3 result=texture(image,TexCoords).rgb*weight[0];
	if(horizontal){
		for(int i=1;i<kernelSize;++i){
			result+=texture(image,TexCoords+vec2(tex_offset.x*i,0.0)).rgb*weight[i];
			result+=texture(image,TexCoords-vec2(tex_offset.x*i,0.0)).rgb*weight[i];
		}
	}
	else{
		for(int i=1;i<kernelSize;++i){
			result+=texture(image,TexCoords+vec2(0.0,tex_offset.y*i)).rgb*weight[i];
			result+=texture(image,TexCoords-vec2(0.0,tex_offset.y*i)).rgb*weight[i];
		}
	}
	FragColor = vec4(result,1.0);
}