#version 330 core
out vec4 FragColor;

in vec2 vTexCoord;

uniform bool horizontal;
uniform sampler2D uTex0;

const float gWeights2[2] ={
   0.44908,
   0.05092
};
const float gOffsets2[2] ={
   0.53805,
   2.06278
};
const float gWeights4[4] ={
   0.24961,
   0.19246,
   0.05148,
   0.00645
};
const float gOffsets4[4] ={
   0.64434,
   2.37885,
   4.29111,
   6.21661
};
const float gWeights6[6] ={
    0.16501,
    0.17507,
    0.10112,
    0.04268,
    0.01316,
    0.00296
};
const float gOffsets6[6] ={
   0.65772,
   2.45017,
   4.41096,
   6.37285,
   8.33626,
   10.30153
};
const float gWeights9[9] ={
   0.10855,
   0.13135,
   0.10406,
   0.07216,
   0.04380,
   0.02328,
   0.01083,
   0.00441,
   0.00157
};
const float gOffsets9[9] ={
   0.66293,
   2.47904,
   4.46232,
   6.44568,
   8.42917,
   10.41281,
   12.39664,
   14.38070,
   16.36501
};
const float gWeights16[16] ={
   0.05991,
   0.07758,
   0.07232,
   0.06476,
   0.05571,
   0.04604,
   0.03655,
   0.02788,
   0.02042,
   0.01438,
   0.00972,
   0.00631,
   0.00394,
   0.00236,
   0.00136,
   0.00075
};
const float gOffsets16[16] ={
   0.66555,
   2.49371,
   4.48868,
   6.48366,
   8.47864,
   10.47362,
   12.46860,
   14.46360,
   16.45860,
   18.45361,
   20.44863,
   22.44365,
   24.43869,
   26.43375,
   28.42881,
   30.42389
};
const float gWeights32[32] ={
   0.02954,
   0.03910,
   0.03844,
   0.03743,
   0.03609,
   0.03446,
   0.03259,
   0.03052,
   0.02830,
   0.02600,
   0.02365,
   0.02130,
   0.01900,
   0.01679,
   0.01469,
   0.01272,
   0.01092,
   0.00928,
   0.00781,
   0.00651,
   0.00537,
   0.00439,
   0.00355,
   0.00285,
   0.00226,
   0.00178,
   0.00138,
   0.00107,
   0.00081,
   0.00062,
   0.00046,
   0.00034
};
const float gOffsets32[32] ={
   0.66640,
   2.49848,
   4.49726,
   6.49605,
   8.49483,
   10.49362,
   12.49240,
   14.49119,
   16.48997,
   18.48876,
   20.48754,
   22.48633,
   24.48511,
   26.48390,
   28.48268,
   30.48147,
   32.48026,
   34.47904,
   36.47783,
   38.47662,
   40.47540,
   42.47419,
   44.47298,
   46.47176,
   48.47055,
   50.46934,
   52.46813,
   54.46692,
   56.46571,
   58.46450,
   60.46329,
   62.46208
};

vec3 GaussianBlur( sampler2D tex0, vec2 centreUV, vec2 pixelOffset )                                                                           
{                                                                                                                                                                    
    vec3 colOut = vec3( 0, 0, 0 );                                                                                                                                   
                                                                                                                                                        
    for( int i = 0; i < 2; i++ )                                                                                                                             
    {                                                                                                                                                                
        vec2 texCoordOffset = gOffsets2[i] * pixelOffset;                                                                                                           
        vec3 col = texture( tex0, centreUV + texCoordOffset ).xyz + texture( tex0, centreUV - texCoordOffset ).xyz;                                                
        colOut += gWeights2[i] * col;                                                                                                                               
    }                                                                                                                                                                
                                                                                                                                                                     
    return colOut;
}

void main()
{
	vec2 size=1.0/textureSize(uTex0,0);
	if(horizontal)
   {
     size.y=0;
   }
   else{
     size.x=0;
   }
   FragColor.rgb=GaussianBlur(uTex0,vTexCoord,size);
   FragColor.a=1;
}