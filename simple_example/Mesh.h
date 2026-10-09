#pragma once


class Mesh
{
public:
    Mesh();
    ~Mesh();

    void draw();

private:
    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;
};