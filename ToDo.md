# Combine Buffers

* [Vulkan Tutorial](https://vulkan-tutorial.com/en/Vertex_buffers/Index_buffer)

You now know how to save memory by reusing vertices with index
buffers. This will become especially important in a future
chapter where we're going to load complex 3D models.

The previous chapter already mentioned that you should allocate
multiple resources like buffers from a single memory allocation,
but in fact you should go a step further. Driver developers
recommend that you also store multiple buffers, like the vertex
and index buffer, into a single VkBuffer and use offsets in
commands like vkCmdBindVertexBuffers. The advantage is that your
data is more cache friendly in that case, because it's closer
together. It is even possible to reuse the same chunk of memory
for multiple resources if they are not used during the same
render operations, provided that their data is refreshed,
of course. This is known as aliasing and some Vulkan functions
have explicit flags to specify that you want to do this

# Custom Allocator

* [Vulkan Tutorial](https://vulkan-tutorial.com/en/Vertex_buffers/Staging_buffer)
* [Allocator Library](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)

It should be noted that in a real world application, you're not
supposed to actually call vkAllocateMemory for every individual
buffer. The maximum number of simultaneous memory allocations is
limited by the maxMemoryAllocationCount physical device limit,
which may be as low as 4096 even on high-end hardware like an
NVIDIA GTX 1080. The right way to allocate memory for a large
number of objects at the same time is to create a custom allocator
that splits up a single allocation among many different objects by
using the offset parameters that we've seen in many functions.

You can either implement such an allocator yourself, or use the
VulkanMemoryAllocator library provided by the GPUOpen initiative.
However, for this tutorial it's okay to use a separate allocation
for every resource, because we won't come close to hitting any of
these limits for now.

# Double Buffer Meshes & Graphics

* [Video Explanation](https://www.youtube.com/watch?v=YNFaOnhaaso)

Basically, to reduce stuttering, you can have a mesh that you write to 
and a mesh that you render from. If you modify one and then swap them 
when that one is done, you will be able to create new things and modify
meshes without causing performance stutter since the cpu will always be
active.

# Add VK_FORMAT_R8G8B8A8_SRGB Alternatives

* [Vulkan Tutorial](https://vulkan-tutorial.com/en/Texture_mapping/Images)

It is possible that the VK_FORMAT_R8G8B8A8_SRGB format is not supported 
by the graphics hardware. You should have a list of acceptable 
alternatives and go with the best one that is supported. However, 
support for this particular format is so widespread that we'll skip this 
step. Using different formats would also require annoying conversions. 
We will get back to this in the depth buffer chapter, where we'll 
implement such a system.

# Enable Best Practice Validation

Inadequate descriptor pools are a good example of a problem that the 
validation layers will not catch: As of Vulkan 1.1, 
vkAllocateDescriptorSets may fail with the error code 
VK_ERROR_POOL_OUT_OF_MEMORY if the pool is not sufficiently large, but 
the driver may also try to solve the problem internally. This means that 
sometimes (depending on hardware, pool size and allocation size) the 
driver will let us get away with an allocation that exceeds the limits 
of our descriptor pool. Other times, vkAllocateDescriptorSets will fail 
and return VK_ERROR_POOL_OUT_OF_MEMORY. This can be particularly 
frustrating if the allocation succeeds on some machines, but fails on 
others.

Since Vulkan shifts the responsibility for the allocation to the driver, 
it is no longer a strict requirement to only allocate as many 
descriptors of a certain type 
(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, etc.) as specified by the 
corresponding descriptorCount members for the creation of the descriptor 
pool. However, it remains best practise to do so, and in the future, 
VK_LAYER_KHRONOS_validation will warn about this type of problem if you 
enable Best Practice Validation.

We should search the web to find out how to enable this since support 
has probably been added by now (this tutorial was created like 5 years
ago).

# Re-implement the Transfer Queue

* [Here is where it assumes we want to use one queue](https://vulkan-tutorial.com/Texture_mapping/Images)
* [Here is where it recommends making a transfer queue](https://vulkan-tutorial.com/Vertex_buffers/Staging_buffer)

We had to remove the transfer queue because the tutorial was written 
with only 1 queue in mind. We should find out how to use a transfer 
queue as well as a graphics and present queue.

# Clean up the code

Right now the code is pretty thrown together and is all in 
GraphicsRunner.

### We need to:

* Create a dependency tree, so we can visualize what depends on what and 
so that we can separate everything
* Most likely we will want to put every tracked resource into its own 
wrapper class which will handle instantiation and destruction of the 
resource
  * This will make it easy to see what depends on what since each object
  will have its creation managed
* Clearly label all functions and classes. Should go for a class & 
method approach for functionality rather than the imperative approach 
we've been following

# Add Image Resizing

* [Vulkan Tutorial](https://vulkan-tutorial.com/en/Generating_Mipmaps)
* [STB Image Resize](https://github.com/nothings/stb/blob/master/stb_image_resize2.h)

You could implement a function that searches common texture image 
formats for one that does support linear blitting, or you could 
implement the mipmap generation in software with a library like 
stb_image_resize. Each mip level can then be loaded into the image in 
the same way that you loaded the original image.

It should be noted that it is uncommon in practice to generate the 
mipmap levels at runtime anyway. Usually they are pre-generated and 
stored in the texture file alongside the base level to improve loading 
speed. Implementing resizing in software and loading multiple levels 
from a file is left as an exercise to the reader.
