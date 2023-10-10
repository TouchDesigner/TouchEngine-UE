# How to work with TOPs

You can work with TOPs in a similar fashion that you are already working with textures across Unreal.

Sync modes can have an impact on how you work with TOPs. You might want to use the following method exposed in your TouchEngine Actor blueprints.

- Keep Frame Texture: Keeps the frame texture retrieved from Get TouchEngine Output. When retrieving a TOP, Get TouchEngine Output returns a temporary texture that will go back into a texture pool after another value has been retrieved from TouchEngine, for performance. If the texture needs to be kept alive for longer, this function needs to be called to ensure the frame texture is removed from the pool and will not be overriden.

Additionaly, when changing an attribute of the texture sampler type, such as the filtering mode, you might want to use the following method exposed in your TouchEngine Actor blueprints.

- Refresh Texture Sampler: Force the recreation of the internal Texture Samplers based on the current value of the Texture Filter, AddressX, AddressY, AddressZ, and MipBias. This can be called on any type of textures (even the ones not created by TouchEngine), but it might not work on all types if they have specific implementations. Returns true if the operation was successful (the Texture and its resource were valid)

## Texture format, sampler, materials

When working with TOPs, or with textures in general between UE and TouchEngine, it is important to set texture formats, Material and Samplers according to the type of content being used, rendered / displayed and passed in / out of Unreal / TouchEngine.

As a general rule of thumb, textures that are not meant to carry data (i.e. textures that are meant to be displayed), and textures that are not meant to be used with high precision, should be set to Color / sRGB.

> **⚠️ Important Note** - By default, TouchDesigner TOPs are not sRGB. More details here: https://docs.derivative.ca/SRGB
>
> That being said, you can assume that any 8bit texture you send out of TouchEngine to Unreal is sRGB.
> Because Unreal will apply a gamma correction to any texture received, what you might see on screen might not be what you see on screen in TouchDesigner.
> Therefore, you will have to convert back this texture to linear space using `LinearTosRGB`.

How do you work with this mix of texture formats and Unreal sampler settings ?

You can either make sure to select sRGB when appropriate at the TOP level, in your .tox, or in Unreal, make sure to select the appropriate sampler as well as convert from / to linear when necessary in your materials using `sRGBToLinear` or `LinearTosRGB`.

If you want to get a better idea of how to work around those issues, look at the Materials used in our Sample project, for all the samples displaying content from TOPs.