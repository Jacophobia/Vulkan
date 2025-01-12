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


