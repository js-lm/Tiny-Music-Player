#pragma once

#include <raylib.h>

#include <string>
#include <memory>
#include <array> 
#include <stdexcept>

#include <libid3tag/id3tag.h>

struct AudioTags{
    std::string title;
    std::string artist;
};

namespace _{

    struct ID3FileDeleter{
        void operator()(id3_file *file) const{ if(file) id3_file_close(file);}
    };

    inline std::string getFrameText(const id3_frame *frame){
        if(!frame) return "";
        
        /* Check the fields */ {
            for(size_t i{0}; i < frame->nfields; i++){

                if(frame->fields[i].type == ID3_FIELD_TYPE_TEXTENCODING) continue;
                
                if(frame->fields[i].type == ID3_FIELD_TYPE_STRINGLIST){
                    size_t numberOfStrings{id3_field_getnstrings(&frame->fields[i])};
                    
                    if(numberOfStrings > 0){
                        const id3_ucs4_t *ucs4String{id3_field_getstrings(&frame->fields[i], 0)};

                        if(ucs4String){
                            std::unique_ptr<id3_utf8_t[], decltype(&free)> utf8String(
                                id3_ucs4_utf8duplicate(ucs4String), &free);
                            if(utf8String){
                                return std::string(reinterpret_cast<char*>(utf8String.get()));
                            }
                        }
                    }

                }else if(frame->fields[i].type == ID3_FIELD_TYPE_STRING){
                    const id3_ucs4_t *ucs4String{id3_field_getstring(&frame->fields[i])};

                    if(ucs4String){
                        std::unique_ptr<id3_utf8_t[], decltype(&free)> utf8String(
                            id3_ucs4_utf8duplicate(ucs4String), &free);

                        if(utf8String){
                            return std::string(reinterpret_cast<char*>(utf8String.get()));
                        }
                    }

                }else if(frame->fields[i].type == ID3_FIELD_TYPE_LATIN1){

                    const id3_latin1_t *latin1String{id3_field_getlatin1(&frame->fields[i])};

                    if(latin1String){
                        return std::string(reinterpret_cast<const char*>(latin1String));
                    }
                }
            }
        } /* Check the fields */
  
        return "";
    }

    
}; // namespace _

inline AudioTags GetMusicMetadata(const std::string &filePath){
    AudioTags tags;
    
    std::unique_ptr<id3_file, _::ID3FileDeleter> file(
        id3_file_open(filePath.c_str(), ID3_FILE_MODE_READONLY)
    );
    
    if(!file){
        tags.title = GetFileNameWithoutExt(filePath.c_str());
        tags.artist = "";

        return tags;
    }
    
    id3_tag *tag{id3_file_tag(file.get())};
    if(tag){
        /* Tying to get the title (TIT2) */ {
            id3_frame *titleFrame{id3_tag_findframe(tag, "TIT2", 0)};

            if(titleFrame) tags.title = _::getFrameText(titleFrame);

        } /* Tying to get the title (TIT2) */

        /* Tying to get the artist (TPE1) */ {
            id3_frame *artistFrame{id3_tag_findframe(tag, "TPE1", 0)};
            if(artistFrame) tags.artist = _::getFrameText(artistFrame);
        } /* Tying to get the artist (TPE1) */
    }

    if(tags.title.empty()) tags.title = GetFileNameWithoutExt(filePath.c_str());
    
    return tags;
}