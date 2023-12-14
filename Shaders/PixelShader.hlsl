// an ultra simple hlsl pixel shader
// TODO: Part 3A
#pragma pack_matrix(row_major)

struct OutputToRasterizer
{
    
    float4 posH : SV_POSITION;
    float3 posW : WORLD; //is this just the POSITION?
    float3 normW : NORMAL;
    float3 uvwW : UVW;
    uint instID : SV_InstanceID;
};

struct PS_INPUT
{
    OutputToRasterizer pInput;
};

struct OBJ_ATTRIBUTES
{
    float3 Kd; // diffuse reflectivity
    float d; // dissolve (transparency) 
    float3 Ks; // specular reflectivity
    float Ns; // specular exponent
    float3 Ka; // ambient reflectivity
    float sharpness; // local reflection map sharpness
    float3 Tf; // transmission filter
    float Ni; // optical density (index of refraction)
    float3 Ke; // emissive reflectivity
    uint illum; // illumination model
};

cbuffer SceneData : register(b0)
{
    float4 vec_SunDirection, vec_SunColor;
    float4x4 mat_View, mat_Projection;
    float4 sunAmbient, cameraPos;
};

cbuffer MeshData : register(b1)
{
    float4x4 mat_World[200];
    OBJ_ATTRIBUTES material[200];
};

cbuffer IdData : register(b2)
{
    float4 tmIDS;
};
// TODO: Part 4B 
// TODO: Part 4C 
// TODO: Part 4F 

//level model has a materialcount, materialstart is in tmIDS
float4 main(PS_INPUT psInput) : SV_TARGET
{
    
    OutputToRasterizer vOutput = psInput.pInput;
    /*
    Lighting Formula for directional light
    Normal into world space – done
    Dot product of the lightDirection and the normal vertex and clamp
    Get  ratio of a clamp of the dotproduct plus the ambient light
    Result = lightratio * lightcolor * surface color
    */
    
    uint index = int(tmIDS.y); //+ vOutput.instID; //this needs to be added with the current insID?
    
    //material 0 is used for testing
                        
    //Compute direct light energy based on light type and surface normal/position                                             
    float direct = clamp(dot(normalize(-vec_SunDirection.xyz), normalize(vOutput.normW)), 0, 1);

    //sunambient and the KA, Compute indirect light from ambient light attribute
    float3 indirect = sunAmbient * material[index].Kd; //tmIDS.y needs an offest to add to it
    
    //Specular Component
    float3 viewDir = normalize(cameraPos - vOutput.posW);

    //compute the exact vector reflected from the surface and compare that to your view vector.
          
    float3 halfVector = normalize((-vec_SunDirection.xyz + viewDir));
    float intensity = max(pow(clamp(dot(normalize(vOutput.normW), halfVector), 0, 1), material[index].Ns + 0.0000001f),
    0);
    
    float reflectedlight = vec_SunColor.xyz * material[index].Ks * intensity;
    

    //camp(totalDirect + totalindirect) * diffuse + totalreflected + emmissive;
    float3 result = clamp((direct + indirect), 0, 1) * material[index].Kd + reflectedlight + material[index].Ke;

    
    return float4(result, 1); //float4(material.Kd, 1); // TODO: Part 1A (optional) 

}