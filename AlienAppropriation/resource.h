#ifndef RESOURCE_H_
#define RESOURCE_H_

#include <string>
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace game {

    // Possible resource types
    typedef enum Type { Material, PointSet, Mesh, Texture, CubeMap } ResourceType;

    // Class that holds one resource
    class Resource {

        private:
            ResourceType mType; // Type of resource
            std::string mName; // Reference name
            union {
                struct {
                    GLuint mResource; // OpenGL handle for resource
                };
                struct {
                    GLuint mArrayBuffer; // Buffers for geometry
                    GLuint mElementArrayBuffer;
                };
            };
            GLsizei mSize; // Number of primitives in geometry

        public:
            Resource(ResourceType type, std::string name, GLuint resource, GLsizei size);
            Resource(ResourceType type, std::string name, GLuint array_buffer, GLuint element_array_buffer, GLsizei size);
            ~Resource();
            ResourceType getType(void) const;
            const std::string getName(void) const;
            GLuint getResource(void) const;
            GLuint getArrayBuffer(void) const;
            GLuint getElementArrayBuffer(void) const;
            GLsizei getSize(void) const;

    }; // class Resource

} // namespace game

#endif // RESOURCE_H_
