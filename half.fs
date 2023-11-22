#version 330 core
out vec4 FragColor;

// texture sampler
uniform sampler2D texture1;

void main()
{
	vec2 vTexCoord = gl_FragCoord.xy*2 *(1.0/textureSize(texture1, 0));

    // need to use textureOffset here
    vec3 col0 = textureOffset(texture1, vTexCoord, ivec2( -1,  0 ) ).xyz;
    vec3 col1 = textureOffset(texture1, vTexCoord, ivec2(  1,  0 ) ).xyz;
    vec3 col2 = textureOffset(texture1, vTexCoord, ivec2(  0, -1 ) ).xyz;
    vec3 col3 = textureOffset(texture1, vTexCoord, ivec2(  0,  1 ) ).xyz;

    vec3 col = (col0+col1+col2+col3) * 0.25;

    FragColor = vec4( col.xyz, 1.0 );
}