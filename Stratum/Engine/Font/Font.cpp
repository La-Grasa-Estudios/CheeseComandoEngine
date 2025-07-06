#include "Font.h"

#include <Core/Logger.h>
#include <Core/JobManager.h>

#include "freetype/ft2build.h"
#include FT_FREETYPE_H  

#include "Thirdparty/rect/finders_interface.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "VFS/stb/stb_image_write.h"
#include "VFS/ZVFS.h"

using namespace ENGINE_NAMESPACE;

Font::Font(const std::string_view& path)
{
    mDescriptorHandle = 0xFFFFFFFF;
    std::thread t {
        [path, this]()
        {
            
        }
    };

    using namespace rectpack2D;

    const uint32_t size = 32;
    FT_Library ft;
    if (FT_Init_FreeType(&ft))
    {
        ErrorLogCs("ZGUI", "Could not init FreeType Library");
        return;
    }
    FT_Face face;

    RefBinaryStream stream = ZVFS::GetFile(path.data());

    if (FT_New_Memory_Face(ft, stream->As<FT_Byte>(), stream->Size(), 0, &face))
    {
        ErrorLogCs("ZGUI", "Failed to load font");
        return;
    }

    FT_Select_Charmap(face, ft_encoding_unicode);
    FT_Set_Pixel_Sizes(face, 0, size);

    FT_GlyphSlot slot = face->glyph;

    constexpr bool allow_flip = false;
    const auto runtime_flipping_mode = flipping_option::DISABLED;

    using spaces_type = rectpack2D::empty_spaces<allow_flip, default_empty_spaces>;
    using rect_type = output_rect_t<spaces_type>;

    uint32_t successfull_rects = 0;
    const auto max_side = 16384;
    const auto discard_step = -4;

    auto report_successful = [](rect_type& r) {
        return callback_result::CONTINUE_PACKING;
        };

    auto report_unsuccessful = [](rect_type& r) {
        return callback_result::ABORT_PACKING;
        };

    std::vector<rect_type> rectangles;
    std::vector<char*> glyphData;
    std::vector<int> charIndexes;

    for (int i = 0; i < 0x01FF; i++) {
        if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
            fprintf(stderr, "Loading character %c failed!\n", i);
            continue;
        }

        FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);

        if (!slot->bitmap.width)
        {
            continue;
        }

        charIndexes.push_back(i);

        size_t size = slot->bitmap.width * slot->bitmap.rows;
        char* data = new char[size];
        memcpy(data, slot->bitmap.buffer, size);

        rectangles.push_back(rect_xywh(0, 0, slot->bitmap.width, slot->bitmap.rows));
        glyphData.push_back(data);

        CharGlyph glyph;

        glyph.advance_x = slot->advance.x; // Convert from 26.6 fixed point to pixels
        glyph.advance_y = slot->advance.y; // Convert from 26.6 fixed point to pixels
        glyph.bearing_x = slot->bitmap_left; // Left bearing in pixels
        glyph.bearing_y = slot->bitmap_top; // Top bearing in pixels

        mGlyphs[i] = glyph;
        mGlyphs[i].rect.w = slot->bitmap.width;
        mGlyphs[i].rect.h = slot->bitmap.rows;
    }

    const auto result_size = find_best_packing_dont_sort<spaces_type>(
        rectangles,
        make_finder_input(
            max_side,
            discard_step,
            report_successful,
            report_unsuccessful,
            runtime_flipping_mode
        )
    );

    size_t sz = result_size.w * result_size.h;
    char* d = new char[sz];
    memset(d, 0, sz);

    for (int i = 0; i < glyphData.size(); i++)
    {
        auto rect = rectangles[i];
        size_t offset = rect.x + rect.y * result_size.w;
        size_t rowPitch = result_size.w;
        char* buff = glyphData[i];

        for (int x = 0; x < rect.w; x++)
        {
            for (int y = 0; y < rect.h; y++)
            {
                size_t o = offset + (x + y * rowPitch);
                size_t j = x + y * rect.w;
                d[o] = buff[j];
            }
        }

        delete[] buff;

        mGlyphs[charIndexes[i]].rect.x = rect.x;
        mGlyphs[charIndexes[i]].rect.y = rect.y;
    }

    mResultSize = { result_size.w, result_size.h };
    mBuffer = d;
    mIsFontReady = true;

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    // No need to keep this thread
    // Doesn't use any global resources just local
    t.detach();
}

Font::~Font()
{
	delete[] mBuffer;
}

CharGlyph* Font::GetGlyph(wchar_t c)
{
    return &mGlyphs[c];
}

glm::ivec2 Font::GetFontAtlasSize() const
{
    return mResultSize;
}

char* Font::GetBuffer() const
{
    return mBuffer;
}

void Font::SetDescriptorHandle(uint32_t handle)
{
	mDescriptorHandle = handle;
}

uint32_t Font::GetDescriptorHandle() const
{
    return mDescriptorHandle;
}

bool Font::IsFontReady() const
{
    return mIsFontReady;
}

void Font::WaitForFontReady() const
{
    while (!IsFontReady())
    {
        std::this_thread::yield();
    }
}
