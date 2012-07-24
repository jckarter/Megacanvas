//
//  View.cpp
//  Megacanvas
//
//  Created by Joe Groff on 6/29/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#include <cmath>

#include "Engine/View-priv.hpp"

namespace Mega {
    using namespace std;
    
    MEGA_PRIV_DTOR(View)

    Priv<View>::Priv(Canvas c)
    : 
    canvas(c), center(0.0, 0.0), zoom(1.0), viewport(0.0, 0.0), good(false), uniforms(),
    program(string(shaderPath) + "/megacanvas"),
    eltCount(0), tilesTextureSize(4096)
    {
    }

    Owner<View> View::create(Canvas c)
    {
        return createOwner<View>(c);
    }
    
    static bool defaultFramebufferIsSRGB()
    {
        GLint param;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &param);
        GLenum attachment = param == 0 ? GL_BACK_LEFT : GL_COLOR_ATTACHMENT0;
        glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &param);
        MEGA_ASSERT_GL_NO_ERROR;
        return param == GL_SRGB;
    }

    bool View::prepare(std::string *outError)
    {
        glEnable(GL_FRAMEBUFFER_SRGB);
        glEnable(GL_BLEND);
        glEnable(GL_CULL_FACE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
        MEGA_ASSERT_GL_NO_ERROR;
        assert(defaultFramebufferIsSRGB());
        
        if (!$.program.compile(outError))
            return false;
        glBindFragDataLocation($.program, 0, "color");
        if (!$.program.link(outError))
            return false;
        
        if (!getUniformLocations($.program, &$.uniforms)) {
            *outError = "unable to set up uniforms for GLSL program";
            return false;
        }
        
        $.tilesTexture.gen();
        $.meshBuffer.gen();
        $.eltBuffer.gen();
        $.pixelBuffers.gen();
        $.meshArray.gen();
        
        $$.bindState();
        
        glActiveTexture(GL_TEXTURE0);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        MEGA_ASSERT_GL_NO_ERROR;
        
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_SRGB8_ALPHA8, 
                     $.tilesTextureSize, $.tilesTextureSize, $.canvas.layers().size(),
                     0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        MEGA_ASSERT_GL_NO_ERROR;
        
        glUniform1f($.uniforms.tilesTextureSize, $.tilesTextureSize);
        //glUniform1i($.uniforms.tilesTexture, 0);
        MEGA_ASSERT_GL_NO_ERROR;
        
        $.updateMesh();
        $.updateViewport();
        $.updateCenter();
        
        $.good = true;
        return true;
    }
    
    void Priv<View>::updateMesh()
    {        
        auto layers = $.canvas.layers();
        size_t layerCount = layers.size();
        unique_ptr<ViewVertex[]> mesh(new ViewVertex[4 * layerCount]);
        unique_ptr<GLushort[]> elts(new GLushort[6 * layerCount]);
        ViewVertex *meshp = mesh.get();
        GLushort *eltsp = elts.get();
        
        for (size_t i = 0; i < layerCount; ++i) {
            Vec origin = layers[i].origin();
            Vec parallax = layers[i].parallax();
            float ox = float(origin.x), oy = float(origin.y);
            float px = float(parallax.x), py = float(parallax.y);
            float layer = float(i);
            
            *meshp++ = (ViewVertex){{-1.0f, -1.0f}, {ox, oy}, {px, py}, layer, 0.0f};
            *meshp++ = (ViewVertex){{ 1.0f, -1.0f}, {ox, oy}, {px, py}, layer, 0.0f};
            *meshp++ = (ViewVertex){{-1.0f,  1.0f}, {ox, oy}, {px, py}, layer, 0.0f};
            *meshp++ = (ViewVertex){{ 1.0f,  1.0f}, {ox, oy}, {px, py}, layer, 0.0f};

            *eltsp++ = 4*i + 0;
            *eltsp++ = 4*i + 1;
            *eltsp++ = 4*i + 2;
            *eltsp++ = 4*i + 2;
            *eltsp++ = 4*i + 1;
            *eltsp++ = 4*i + 3;
        }
        
        MEGA_ASSERT_GL_NO_ERROR;
        glBufferData(GL_ARRAY_BUFFER, sizeof(ViewVertex)*4*layerCount, mesh.get(), GL_STATIC_DRAW);
        MEGA_ASSERT_GL_NO_ERROR;
        GLint binding;
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &binding);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort)*6*layerCount, elts.get(), GL_STATIC_DRAW);
        MEGA_ASSERT_GL_NO_ERROR;
        
        $.eltCount = 6 * layerCount;
        
        bindVertexAttributes<ViewVertex>($.program);
        MEGA_ASSERT_GL_NO_ERROR;
    }
    
    void Priv<View>::updateViewport()
    {
        Vec zoomedView = $.viewport / $.zoom;
        glUniform2f($.uniforms.viewport, zoomedView.x, zoomedView.y);
        MEGA_ASSERT_GL_NO_ERROR;
    }
    
    void Priv<View>::updateCenter()
    {
        glUniform2f($.uniforms.center, $.center.x, $.center.y);
        MEGA_ASSERT_GL_NO_ERROR;
    }

    void View::bindState()
    {
        glUseProgram($.program);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, $.tilesTexture);
        
        glBindVertexArray($.meshArray);

        glBindBuffer(GL_ARRAY_BUFFER, $.meshBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, $.eltBuffer);
        
        MEGA_ASSERT_GL_NO_ERROR;
    }
    
    void View::resize(double width, double height)
    {
        $.viewport = Vec(width, height);
        $.updateViewport();
    }
    
    void View::render()
    {
        assert($.good);
        
        glDrawElements(GL_TRIANGLES, $.eltCount, GL_UNSIGNED_SHORT, nullptr);
        MEGA_ASSERT_GL_NO_ERROR;
    }
    
    MEGA_PRIV_GETTER(View, center, Vec)
    void View::center(Mega::Vec c)
    {
        $.center = c;
        $.updateCenter();
    }
    
    void View::moveCenter(Mega::Vec c)
    {
        $.center += c;
        $.updateCenter();
    }
    
    MEGA_PRIV_GETTER(View, zoom, double)
    void View::zoom(double z)
    {
        $.zoom = std::max(0.5, z);
        $.updateViewport();
    }
    
    void View::moveZoom(double z)
    {
        $$.zoom($.zoom + z);
    }
    
    Vec View::viewToLayer(Mega::Vec viewPoint, Layer l)
    {
        assert(false);
    }
}