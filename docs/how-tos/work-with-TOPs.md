# How to work with TOPs

You can work with TOPs in a similar fashion that you are already working with textures across Unreal.

Sync modes can have an impact on how you work with TOPs. You might want to use the following method exposed in your TouchEngine Actor blueprints.

- Keep Frame Texture: Keeps the frame texture retrieved from Get TouchEngine Output. When retrieving a TOP, Get TouchEngine Output returns a temporary texture that will go back into a texture pool after another value has been retrieved from TouchEngine, for performance. If the texture needs to be kept alive for longer, this function needs to be called to ensure the frame texture is removed from the pool and will not be overriden.

Additionaly, when changing an attribute of the texture sampler type, such as the filtering mode, you might want to use the following method exposed in your TouchEngine Actor blueprints.

- Refresh Texture Sampler: Force the recreation of the internal Texture Samplers based on the current value of the Texture Filter, AddressX, AddressY, AddressZ, and MipBias. This can be called on any type of textures (even the ones not created by TouchEngine), but it might not work on all types if they have specific implementations. Returns true if the operation was successful (the Texture and its resource were valid)

## Texture format, sampler, materials

When working with TOPs, or with textures in general between UE and TouchEngine, it is important to set Material and Samplers according to the type of content being used, rendered / displayed and passed in / out of Unreal / TouchEngine.

As a general rule of thumb, textures that are not meant to be sampled, and textures that are not meant to be used with high precision, should be set to Color / sRGB. 

In our samples project, the first few samples showcasing how TouchEngine loads and how it performs are displaying a simple texture that is 8bit RGBA in the TouchDesigner custom COMP. Applied on the square, it's a Material with a Color sampler and a Material Domain set to UI. We set the Material Domain to UI to have a simple Material with no shading, similar to the Constant MAT in TouchDesigner.

Textures that you want to use with high precision (16bit, 32Bit.. etc), such as a texture to be used for a Normal Map, or some complex sampling in a dynamic material, or in Niagara, should preferably be used with an appropriate sampler or a Linear Color sampler.