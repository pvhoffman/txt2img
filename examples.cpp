#include "stdafx.h"

#include "txt2img.h"

int main (int argc, char const* argv[])
{
    txt2img::TextImage ti;

    ti.inp_params.fontParams.width = ti.inp_params.fontParams.height = 8.0f;
    ti.inp_params.graphicsParams.halign = txt2img::TextImage::input_params_t::alignCenter;
    ti.inp_params.graphicsParams.valign = txt2img::TextImage::input_params_t::alignCenter;
    ti.inp_params.graphicsParams.rotate = 22.0;

    ti.out_params.width = ti.out_params.height = 50.0;
    ti.out_params.type_out = txt2img::TextImage::pgmraw;

    strcpy(ti.out_params.fileName, "c:\\temp\\out.pgm");

    ti.emit("A");

    return 0;
}
