#include "stdafx.h"

#include "txt2img.h"


namespace txt2img {

namespace local {

static const double RASTER_RESOLUTION = 96.0;
static const double POSTSCRIPT_RESOLUTION = 72.0;

static const char* GHOSTSCRIPT_REGISTRY_PATH = ("SOFTWARE\\GPL Ghostscript");

int default_gsdll_callback(int message, char *str, unsigned long count);
static const char* ghostscript_device_from_user(const TextImage::TOutputType& tout);


class Ghostscript
{
private:
        HMODULE _dll_handle;

        void _init();
        void _read_ghostscript_registry();
        HKEY _get_ghostscript_registry_key();
public: 
        enum {
            GSDLL_STDIN    = 1
                , GSDLL_STDOUT = 2
                , GSDLL_DEVICE = 3
                , GSDLL_SYNC   = 4
                , GSDLL_PAGE   = 5
                , GSDLL_SIZE   = 6
                , GSDLL_POLL   = 7
        };


        int (__stdcall *gsdll_revision)(char **product, char **copyright, long *gs_revision, long *gs_revisiondate);
        int (__stdcall *gsdll_init)( int (*gsdll_callback)(int message, char* str, unsigned long count), void* hwnd, int argc, char** argv);
        int (__stdcall *gsdll_execute_begin)(void);
        int (__stdcall *gsdll_execute_cont)(const char *str, int len);
        int (__stdcall *gsdll_execute_end)(void);
        int (__stdcall *gsdll_lock_device)(unsigned char *device, int flag);
        int (__stdcall *gsdll_exit)(void);

        std::string ghostscript_include;
        std::string ghostscript_path;

        Ghostscript();
        virtual ~Ghostscript();
};
//---------------------------------------------------------
Ghostscript::Ghostscript()
{
    _dll_handle = 0;
    _init();
}
Ghostscript::~Ghostscript()
{
    if(_dll_handle){
        ::FreeLibrary(_dll_handle);
        _dll_handle = 0;
    }
}
//---------------------------------------------------------
void Ghostscript::_init()
{
    if(_dll_handle){
        ::FreeLibrary(_dll_handle);
        _dll_handle = 0;
    }
    _read_ghostscript_registry();

    _dll_handle = ::LoadLibrary(ghostscript_path.c_str());
    if(!_dll_handle){
        throw std::runtime_error(std::string("Cannot load ghostscript dll from ") + ghostscript_path);
    }

    gsdll_revision       = (int (__stdcall*)(char**, char**, long*, long*))::GetProcAddress(_dll_handle, "gsdll_revision");
    gsdll_init           = (int (__stdcall*)(int (*)(int,char *,unsigned long),void *,int,char **))::GetProcAddress(_dll_handle, "gsdll_init");
    gsdll_execute_begin  = (int (__stdcall*)(void))::GetProcAddress(_dll_handle, "gsdll_execute_begin");
    gsdll_execute_cont   = (int (__stdcall*)(const char *,int))::GetProcAddress(_dll_handle, "gsdll_execute_cont");
    gsdll_execute_end    = (int (__stdcall*)(void))::GetProcAddress(_dll_handle, "gsdll_execute_end");
    gsdll_lock_device    = (int (__stdcall*)(unsigned char*, int))::GetProcAddress(_dll_handle, "gsdll_lock_device");
    gsdll_exit           = (int (__stdcall*)(void))::GetProcAddress(_dll_handle, "gsdll_exit");

    if (!gsdll_revision
            || !gsdll_init
            || !gsdll_execute_begin
            || !gsdll_execute_cont
            || !gsdll_execute_end
            || !gsdll_lock_device
            || !gsdll_exit){
        throw std::runtime_error("One or more exports could not be resolved in gsdll32.dll");
    }
}
//---------------------------------------------------------
HKEY Ghostscript::_get_ghostscript_registry_key()
{
        HKEY key;
        LONG l1, l2;
        char buff1[MAX_PATH], buff2[MAX_PATH];
        unsigned long dd;

        l1 = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, GHOSTSCRIPT_REGISTRY_PATH
                            , 0UL, KEY_READ, &key);
        if (l1 != ERROR_SUCCESS)
                throw std::runtime_error("Cannot open Ghostscript Registry Key.");

        for (l1=0, l2 = ERROR_SUCCESS;l2==ERROR_SUCCESS;l1++) {
                FILETIME ft;
                char version[MAX_PATH];
                dd = MAX_PATH;
                l2 = ::RegEnumKeyEx(key, l1, version, &dd, NULL, NULL, NULL, &ft);
                if (l2 == ERROR_SUCCESS)
                        strcpy(buff1, version);
        }
        ::RegCloseKey(key);

        if (!buff1[0])
                throw std::runtime_error("Cannot locate ghostscript installation in registry.");

        sprintf(buff2, "%s\\%s", GHOSTSCRIPT_REGISTRY_PATH, buff1);

        l1 = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, buff2, 0UL, KEY_READ, &key);
        if (l1 != ERROR_SUCCESS)
                throw std::runtime_error("Cannot open ghostscript registry key.");
        return key;

}
//---------------------------------------------------------
void Ghostscript::_read_ghostscript_registry()
{
    char buffer[1024];
    unsigned long l1, l2;
    HKEY key = _get_ghostscript_registry_key();

    l2 = 1024;
    l1 = ::RegQueryValueEx(key, "GS_DLL", NULL, NULL, (unsigned char*)buffer, &l2);
    if (l1 != ERROR_SUCCESS)
        throw std::runtime_error("Cannot read GS_DLL value from registry.");
    ghostscript_path.assign(buffer);

    l2 = 1024;
    l1 = ::RegQueryValueEx(key, "GS_LIB", NULL, NULL, (unsigned char*)buffer, &l2);
    if (l1 != ERROR_SUCCESS)
        throw std::runtime_error("Cannot read GS_LIB value from registry.");
    ghostscript_include.assign(buffer);

    ::RegCloseKey(key);

}
//---------------------------------------------------------
int default_gsdll_callback(int message, char *str, unsigned long count)
{
    char* p = 0;
    switch (message) {
        case Ghostscript::GSDLL_STDIN:
            p = fgets(str, count, stdin);
            if (p)
                return strlen(str);
            else
                return 0;
        case Ghostscript::GSDLL_STDOUT:
                if (str != (char *)NULL)
                    fwrite(str, 1, count, stdout);
                return count;
        case Ghostscript::GSDLL_DEVICE:
        case Ghostscript::GSDLL_SYNC:
        case Ghostscript::GSDLL_PAGE:
        case Ghostscript::GSDLL_SIZE:
        case Ghostscript::GSDLL_POLL:
                return (0);
    }
    return (0);

}
//---------------------------------------------------------
static const char* ghostscript_device_from_user(const TextImage::TOutputType& tout)
{
        static std::string res;

        res.assign("pgmraw");

        switch(tout){
            case TextImage::bmpgray:
                res.assign("bmpgray");
                break;
            case TextImage::bmp16:
                res.assign("bmp16");
                break;
            case TextImage::bmp256:
                res.assign("bmp256");
                break;
        }

        return res.c_str();
}
//---------------------------------------------------------
} // namespace local
//---------------------------------------------------------
TextImage::TextImage() : inp_params(defaultInputParams()), out_params(defaultOutputParams()) 
{
}
//---------------------------------------------------------
TextImage::~TextImage()
{
}
//---------------------------------------------------------
void TextImage::emit(const char* text)
{
    local::Ghostscript gs;

    _init_device(gs);

    _init_page(gs);

    _emit_text(gs,text);

    _end_page(gs);
}
//---------------------------------------------------------
void TextImage::_emit_text(local::Ghostscript& gs, const char* text)
{
    std::stringstream psout;

    psout << "0 0 moveto" << std::endl;
    psout << "0 0 0 1 setcmykcolor" << std::endl;
    psout << "/" << inp_params.fontParams.faceName << " findfont [" << inp_params.fontParams.width << " 0 0 " << inp_params.fontParams.height << " 0 0] makefont setfont" << std::endl;

    const char* cptr = text;

    psout << "(";
    while(cptr && *cptr){
        switch(*cptr){
                case '\\':
                case '(':
                case ')':
                    psout << '\\';
                    break;
        }
        psout << (*cptr);
        cptr++;
    }

    psout << ") ps-halign ps-valign show" << std::endl;

    std::string ps = psout.str();
    int gscode = gs.gsdll_execute_cont(ps.c_str(), ps.length());
    if(gscode){
        throw std::runtime_error("Ghostscript execution failed.");
    }


}
//---------------------------------------------------------
void TextImage::_end_page(local::Ghostscript& gs)
{
    std::stringstream psout;

    psout << "showpage" << std::endl;

    std::string ps = psout.str();
    int gscode = gs.gsdll_execute_cont(ps.c_str(), ps.length());
    if(gscode){
        throw std::runtime_error("Ghostscript execution failed.");
    }

    gs.gsdll_execute_end();
    gs.gsdll_exit();

}
//---------------------------------------------------------
void TextImage::_init_page(local::Ghostscript& gs)
{
    std::stringstream psout;

    psout << "/ps-image-width "  << ((local::POSTSCRIPT_RESOLUTION / local::RASTER_RESOLUTION) * out_params.width)  << " def" << std::endl;
    psout << "/ps-image-height " << ((local::POSTSCRIPT_RESOLUTION / local::RASTER_RESOLUTION) * out_params.height) << " def" << std::endl;

    std::string ps = psout.str();
    int gscode = gs.gsdll_execute_cont(ps.c_str(), ps.length());
    if(gscode){
        throw std::runtime_error("Ghostscript execution failed.");
    }
    psout.str( std::string("") );

    psout << "/ps-halign {" << std::endl;
    switch(inp_params.graphicsParams.halign){
        case input_params_t::alignLeft:
            break;
        case input_params_t::alignCenter:
            psout << "dup stringwidth pop 2.0 div ps-image-width 2.0 div exch sub currentpoint exch pop moveto" << std::endl;
            break;
        case input_params_t::alignRight:
            psout << "dup stringwidth pop ps-image-width exch sub currentpoint exch pop moveto" << std::endl;
            break;
        default:
            break;
    }
    psout << "} def" << std::endl;

    ps = psout.str();
    gscode = gs.gsdll_execute_cont(ps.c_str(), ps.length());
    if(gscode){
        throw std::runtime_error("Ghostscript execution failed.");
    }
    psout.str( std::string("") );

    psout << "/ps-valign {" << std::endl;
    switch(inp_params.graphicsParams.valign){
        case input_params_t::alignBottom:
            break;
        case input_params_t::alignCenter:
            psout << "currentpoint pop ps-image-height 2.0 div moveto" << std::endl; 
            break;
        case input_params_t::alignTop:
            psout << "currentpoint pop ps-image-height " << inp_params.fontParams.height << " sub moveto" << std::endl; 
            break;
    }
    psout << "} def" << std::endl;

    psout << inp_params.graphicsParams.rotate << " rotate" << std::endl;

    ps = psout.str();
    gscode = gs.gsdll_execute_cont(ps.c_str(), ps.length());
    if(gscode){
        throw std::runtime_error("Ghostscript execution failed.");
    }
    psout.str( std::string("") );




}
//---------------------------------------------------------
void TextImage::_init_device(local::Ghostscript& gs)
{
    enum { max_gs_args = 20 };
    int gsargc = 0, gscode, i;
    char* gsargv[max_gs_args] = {0,};

    gsargv[gsargc++] = strdup("textimage");
    gsargv[gsargc++] = strdup("-dBATCH");
    gsargv[gsargc++] = strdup("-dNOPAUSE");
    gsargv[gsargc++] = strdup("-dNOSAFER");
    gsargv[gsargc++] = strdup("-r96");

    gsargv[gsargc]   = (char*)malloc(255);
    sprintf(gsargv[gsargc++], "-sDEVICE=%s", local::ghostscript_device_from_user(out_params.type_out));

    gsargv[gsargc++] = strdup("-dTextAlphaBits=4");
    gsargv[gsargc++] = strdup("-dGraphicsAlphaBits=4");

    gsargv[gsargc]   = (char*)malloc(255);
    sprintf(gsargv[gsargc++], "-g%dx%d", (int)out_params.width, (int)out_params.height);

    gsargv[gsargc]   = (char*)malloc(255);
    sprintf(gsargv[gsargc++], "-sOutputFile=%s", out_params.fileName);


    gscode = gs.gsdll_init( local::default_gsdll_callback, 0, gsargc, gsargv );

    for(i = 0; i < gsargc; i++){
        free(gsargv[i]);
        gsargv[i] = 0;
    }

    if(gscode){
        throw std::runtime_error("Ghostscript initialization failed.");
    }

    gscode = gs.gsdll_execute_begin();
    if(gscode){
        throw std::runtime_error("Ghostscript execute_begin failed.");
    }
}
//---------------------------------------------------------
TextImage::input_params_t TextImage::defaultInputParams()
{
    input_params_t res;

    res.fontParams.width = res.fontParams.height = 12.0f;
    res.fontParams.encoding = input_params_t::encodeLatin1;
    strcpy(res.fontParams.faceName, "Helvetica");

    res.graphicsParams.xoffset = res.graphicsParams.yoffset = res.graphicsParams.rotate = 0.0f;
    res.graphicsParams.halign = input_params_t::alignCenter;
    res.graphicsParams.valign = input_params_t::alignCenter;

    return res;
}
//---------------------------------------------------------
TextImage::output_params_t TextImage::defaultOutputParams()
{
    output_params_t res;
    res.width = res.height = 40.0f;
    res.type_out = pgmraw;
    strcpy(res.fileName, "out.pgm");
    return res;
}
//---------------------------------------------------------
} //namespace txt2img 

