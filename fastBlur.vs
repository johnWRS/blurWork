#version 330 core
in vec2 Position;

out vec2 vTexCoord;

void main()
{
    gl_Position = vec4( Position.x, Position.y, 0.0, 1.0 );
    vTexCoord = vec2( Position.x * 0.5 + 0.5, Position.y * 0.5 + 0.5 );
}