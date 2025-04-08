#include "LoadTexture.h"
#include "FreeImage.h"


GLuint LoadTexture(const std::string& fname)
{
   GLuint tex_id;

   FIBITMAP* tempImg = FreeImage_Load(FreeImage_GetFileType(fname.c_str(), 0), fname.c_str());
   FIBITMAP* img = FreeImage_ConvertTo32Bits(tempImg);

   FreeImage_Unload(tempImg);

   GLuint w = FreeImage_GetWidth(img);
   GLuint h = FreeImage_GetHeight(img);
   GLuint scanW = FreeImage_GetPitch(img);

   GLubyte* byteImg = new GLubyte[h*scanW];
   FreeImage_ConvertToRawBits(byteImg, img, scanW, 32, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, FALSE);
   FreeImage_Unload(img);

   glGenTextures(1, &tex_id);
   glBindTexture(GL_TEXTURE_2D, tex_id);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, byteImg);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   delete byteImg;

   return tex_id;
}

//Loads cubemap textures in the cross format.
//3 rows, 4 columns with square textures where the Xs are below:
// 0X00
// XXXX
// 0X00
GLuint LoadSkybox(const std::string& fname)
{
    GLuint tex_id;

    FIBITMAP* tempImg = FreeImage_Load(FreeImage_GetFileType(fname.c_str(), 0), fname.c_str());
    FIBITMAP* img = FreeImage_ConvertTo32Bits(tempImg);

    FreeImage_Unload(tempImg);

    GLuint w = FreeImage_GetWidth(img);
    GLuint h = FreeImage_GetHeight(img);
    GLuint scanW = FreeImage_GetPitch(img);

    //TODO: handle case where h > w (vertical cross format)

    const int face_w = w / 4;
    const int face_h = h / 3;

    //+x, -x, +y, -y, +z, -z
    int left[6] = { 2 * face_w,  0,          face_w,     face_w,     face_w,     3 * face_w };
    int top[6] = { face_h,    face_h,     0,          2 * face_h,   face_h,     face_h };
    int right[6] = { 3 * face_w,  face_w,     2 * face_w,   2 * face_w,   2 * face_w,   4 * face_w };
    int bottom[6] = { 2 * face_h,  2 * face_h,   face_h,     3 * face_h,   2 * face_h,   2 * face_h };

    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, tex_id);
    glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    for (int i = 0; i < 6; i++)
    {
        FIBITMAP* faceimg = FreeImage_Copy(img, left[i], top[i], right[i], bottom[i]);

        GLuint face_scanW = FreeImage_GetPitch(faceimg);

        GLubyte* byteImg = new GLubyte[face_h * face_scanW];
        FreeImage_ConvertToRawBits(byteImg, faceimg, face_scanW, 32, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, TRUE); //last arg FALSE set origin of the texture to the lower left
        FreeImage_Unload(faceimg);

        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, face_w, face_h, 0, GL_BGRA, GL_UNSIGNED_BYTE, byteImg);

        delete[] byteImg;
    }
    FreeImage_Unload(img);

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    return tex_id;

}