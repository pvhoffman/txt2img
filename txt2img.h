#ifndef txt2img_H_
#define txt2img_H_

namespace txt2img
{
namespace local { class Ghostscript; }// namespace local;

class TextImage
{
public: // types
    enum { EFontNameMaxLength = 255, EFilenameMaxLength = 255 };

    typedef enum { bmpgray , bmp16, bmp256, pgmraw } TOutputType;



    struct input_params_t {
        typedef enum { alignLeft, alignCenter, alignRight, alignBottom, alignTop } TAlign;
        typedef enum { encodeNone, encodeLatin1 } TEncode;

        struct {
            char faceName[EFontNameMaxLength];
            float width;
            float height;
            TEncode encoding;
        } fontParams;

        struct {
            float xoffset;
            float yoffset;
            float rotate;
            TAlign halign;
            TAlign valign;
        } graphicsParams;
    };

    struct output_params_t {
        TOutputType type_out;
        float width;
        float height;
        char fileName[EFilenameMaxLength];
    };
private: // members
    void _init_device(local::Ghostscript& gs);
    void _init_page(local::Ghostscript& gs);
    void _emit_text(local::Ghostscript& gs, const char* text);
    void _end_page(local::Ghostscript& gs);

public:
    input_params_t   inp_params;
    output_params_t  out_params;

    TextImage();
    virtual ~TextImage();

    void emit(const char* text);

    static input_params_t defaultInputParams();
    static output_params_t defaultOutputParams();
};

} // namespace txt2img

#endif /* txt2img_H_ */

