#pragma once
/*

=== This file is based on "libmediaplayerservice/MovAvInfoDetect.cpp" ===
>>> Source:
https://github.com/SuperOSR/android_frameworks_av/blob/02695310094d8eaae8960bc40ccfa3a368fe7b83/media/libmediaplayerservice/MovAvInfoDetect.cpp

=== This file is based on "libavformat/mov.c" ===
>>> Source:
https://github.com/FFmpeg/FFmpeg/blob/master/libavformat/mov.c

*/

#include "string.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <vector>
#include <string>
#include <inttypes.h>
#include <sstream>

#include <../tiny-mov-library/TinyMovFileLibrary.hpp>

#ifdef _WIN32
#define fseek _fseeki64
#define ftell _ftelli64
#endif

enum AVMediaType
{
    AVMEDIA_TYPE_UNKNOWN = -1,
    AVMEDIA_TYPE_VIDEO = 0,
    AVMEDIA_TYPE_AUDIO = 1,
    AVMEDIA_TYPE_DATA = 2,
    AVMEDIA_TYPE_SUBTITLE = 3,
    AVMEDIA_TYPE_ATTACHMENT = 4,
    AVMEDIA_TYPE_NB = 5
};

#define ALOGV c->string_stream
#define ALOGD c->string_stream

//#define INT_MAX      2147483647
#define MKTAG(a, b, c, d) (a | (b << 8) | (c << 16) | (d << 24))

typedef struct
{
    uint32_t type;
    int64_t offset;
    int64_t size; /* total size (excluding the size and type fields) */
} MOV_atom_t;

typedef struct FrameInfo
{
    uint64_t frame_offset; /* frame offsets in file */
    uint64_t frame_size;   /* frame size in file */
} FrameInfo_t;

typedef struct TrackInfo
{
    TrackInfo() : codec_name("none"), universal_sample_size(-1), codec_type(Unknown) {}

    std::string codec_name;

    uint32_t zraw_raw_version;
    uint32_t zraw_unk0;
    uint32_t zraw_unk1;

    enum TrackCodecType {Audio, Video, Unknown};
    TrackCodecType codec_type;

    int width;
    int height;

    int32_t universal_sample_size;
    std::vector<FrameInfo_t> frames;
} TrackInfo_t;

typedef struct TracksInfo
{
    std::vector<TrackInfo_t> tracks;
    uint32_t creation_time;

    std::string output_log;
} TracksInfo_t;

typedef struct MOVContext
{
    MOVContext()
    {
        fp = nullptr;
        file_size = 0;
        time_scale = 0;
        duration = 0;
        found_moov = 0;
        found_mdat = 0;
        has_video = 0;
        has_audio = 0;
        mdat_count = 0;
        isom = 0;
        current_track_index = 0;
        parse_table = nullptr;
    }

    std::stringstream string_stream;

    FILE *fp;
    int32_t file_size;

    int32_t time_scale;
    int32_t duration;   /* duration of the longest track */
    int32_t found_moov; /* when both 'moov' and 'mdat' sections has been found */
    int32_t found_mdat; /* we suppose we have enough data to read the file */

    int32_t has_video;
    int32_t has_audio;

    int32_t mdat_count;
    int32_t isom; /* 1 if file is ISO Media (mp4/3gp) */

    int current_track_index;
    TracksInfo_t tracks_info;
    
    const struct MOVParseTableEntry *parse_table; /* could be eventually used to change the table */
} MOVContext;

typedef int32_t (*mov_parse_function)(MOVContext *ctx, MOV_atom_t atom);

typedef struct MOVParseTableEntry
{
    uint32_t type;
    mov_parse_function func;
} MOVParseTableEntry;

static int32_t get_byte(FILE *s)
{
    uint8_t t;
    int32_t t1;
    fread(&t, 1, 1, s);
    t1 = (int32_t)t;
    return t1;
}

static void get_buffer(FILE *s, uint8_t *data, uint32_t len)
{
    fread(data, 1, len, s);
    return;
}

static uint32_t get_le16(FILE *s)
{
    uint32_t val;
    val = get_byte(s);
    val |= get_byte(s) << 8;
    return val;
}

static uint32_t get_le24(FILE *s)
{
    uint32_t val;
    val = get_le16(s);
    val |= get_byte(s) << 16;
    return val;
}

static uint32_t get_le32(FILE *s)
{
    uint32_t val;
    val = get_le16(s);
    val |= get_le16(s) << 16;
    return val;
}

//static int32_t get_le64(FILE *s)
//{
//    int32_t val,val_h;
//    val = get_le32(s);
//    val_h = get_le32(s) << 32;
//
//    if(val_h!=0){
//        val = INT_MAX;
//    }
//    return val;
//}

static uint32_t get_be16(FILE *s)
{
    uint32_t val;
    val = get_byte(s) << 8;
    val |= get_byte(s);
    return val;
}

static uint32_t get_be24(FILE *s)
{
    uint32_t val;
    val = get_be16(s) << 8;
    val |= get_byte(s);
    return val;
}

static uint32_t get_be32(FILE *s)
{
    uint32_t val;
    val = get_be16(s) << 16;
    val |= get_be16(s);
    return val;
}

static uint64_t get_be64(FILE *s)
{
    uint32_t val;
    const auto a = get_be32(s);
    const auto b = get_be32(s);
    return ((uint64_t)a << 32) | b;
}

static void FSKIP(FILE *fp, int64_t size)
{
    fseek(fp, size, SEEK_CUR);
}

static void FSEEK(FILE *fp, int64_t offset)
{
    fseek(fp, offset, SEEK_SET);
}

static int64_t FTELL(FILE *fp)
{
    return ftell(fp);
}

int64_t fsize(FILE *s)
{
    if (fseek(s, 0, SEEK_END) != 0)
        return -1;

    const auto size = ftell(s);

    if (fseek(s, 0, SEEK_SET) != 0)
        return -1;

    return size;
}

//--------------------------------------------------------
static int32_t mov_read_default(MOVContext *c, MOV_atom_t atom)
{
    int32_t total_size = 0;
    MOV_atom_t a;
    int32_t i;
    int32_t err = 0;
    FILE *pb;

    pb = c->fp;
    a.offset = atom.offset;

    if (atom.size < 0)
        atom.size = 0x7fffffff;
    while (((total_size + 8) < atom.size) /*&& !feof(pb)*/ && !err)
    {
        a.size = atom.size;
        a.type = 0L;
        if (atom.size >= 8)
        {
            a.size = get_be32(pb);
            a.type = get_le32(pb);
        }
        total_size += 8;
        a.offset += 8;
        if (a.size == 1)
        { /* 64 bit extended size */
            ALOGV << "length may error!" << std::endl;
            a.size = get_be64(pb) - 8;
            a.offset += 8;
            total_size += 8;
        }
        if (a.size == 0)
        {
            a.size = atom.size - total_size;
            if (a.size <= 8)
                break;
        }
        a.size -= 8;
        if (a.size < 0 || a.size > atom.size - total_size)
            break;

        for (i = 0; c->parse_table[i].type != 0L && c->parse_table[i].type != a.type; ++i)
            /* empty */;

        char atom_name[5];
        memset(atom_name, 0x00, sizeof(atom_name));
        *(uint32_t*)atom_name = a.type;

        ALOGV << "type: " << a.type << " (" << atom_name << ")" << " | size: " << std::hex << a.size << std::dec << std::endl;

        if (c->parse_table[i].type == 0)
        { /* skip leaf atoms data */
            FSKIP(pb, a.size);
        }
        else
        {
            int32_t start_pos = ftell(pb);
            int32_t left;
            err = (c->parse_table[i].func)(c, a);
            left = a.size - ftell(pb) + start_pos;
            if (left > 0) /* skip garbage at atom end */
                FSKIP(pb, left);
        }
        a.offset += a.size;
        total_size += a.size;
    }

    if (!err && total_size < atom.size && atom.size < 0x7ffff)
    {
        FSKIP(pb, atom.size - total_size);
    }

    return err;
}

static int32_t mov_read_ftyp(MOVContext *c, MOV_atom_t atom)
{
    ALOGV << "Reading \'ftyp\'" << std::endl;

    FILE *pb;
    uint32_t type;

    pb = c->fp;
    type = get_le32(pb);

    if (type != MKTAG('q', 't', ' ', ' '))
        c->isom = 1;
    get_be32(pb); /* minor version */
    FSKIP(pb, atom.size - 8);
    return 0;
}

static int32_t mov_read_mdat(MOVContext *c, MOV_atom_t atom)
{
    ALOGV << "Reading \'mdat\'" << std::endl;

    FILE *pb;

    pb = c->fp;
    if (atom.size == 0) /* wrong one (MP4) */
        return 0;

    c->mdat_count++;
    c->found_mdat = 1;

    if (c->found_moov)
        return 1; /* found both, just go */
    FSKIP(pb, atom.size);
    return 0; /* now go for moov */
}

static int32_t mov_read_wide(MOVContext *c, MOV_atom_t atom)
{
    int32_t err;
    FILE *pb;

    pb = c->fp;

    if (atom.size < 8)
        return 0; /* continue */
    if (get_be32(pb) != 0)
    { /* 0 sized mdat atom... use the 'wide' atom size */
        FSKIP(pb, atom.size - 4);
        return 0;
    }
    atom.type = get_le32(pb);
    atom.offset += 8;
    atom.size -= 8;
    if (atom.type != MKTAG('m', 'd', 'a', 't'))
    {
        FSKIP(pb, atom.size);
        return 0;
    }
    err = mov_read_mdat(c, atom);
    return err;
}

static int32_t mov_read_moov(MOVContext *c, MOV_atom_t atom)
{
    int32_t err;

    ALOGV << "Reading \'moov\'" << std::endl;

    err = mov_read_default(c, atom);
    c->found_moov = 1;
    if (c->found_mdat)
        return 1; /* found both, just go */
    return 0;     /* now go for mdat */
}

static int32_t mov_read_mvhd(MOVContext *c, MOV_atom_t atom)
{
    ALOGV << "Reading \'mvhd\'" << std::endl;

    FILE *pb = c->fp;
    int32_t version = get_byte(pb); /* version */
    get_byte(pb);
    get_byte(pb);
    get_byte(pb); /* flags */

    if (version == 1)
    {
        get_be64(pb);
        get_be64(pb);
    }
    else
    {
        c->tracks_info.creation_time = get_be32(pb);            /* creation time */
        uint32_t modification_time = get_be32(pb);  /* modification time */
    }
    c->time_scale = get_be32(pb); /* time scale */

    c->duration = (int32_t)((version == 1) ? get_be64(pb) : get_be32(pb)); /* duration */

    FSKIP(pb, 6 + 10 + 36 + 28);

    return 0;
}

static int32_t mov_read_trak(MOVContext *c, MOV_atom_t atom)
{
    ALOGV << "Reading \'trak\'" << std::endl;

    c->current_track_index = c->tracks_info.tracks.size();
    c->tracks_info.tracks.push_back(TrackInfo_t());

    auto res = mov_read_default(c, atom);

    c->current_track_index = -1;

    return res;
}

static int32_t mov_read_tkhd(MOVContext *c, MOV_atom_t atom)
{
    ALOGV << "Reading \'tkhd\'" << std::endl;

    FILE *pb = c->fp;
    int32_t version = get_byte(pb);

    get_byte(pb);
    get_byte(pb);
    get_byte(pb); /* flags */

    if (version == 1)
    {
        get_be64(pb);
        get_be64(pb);
    }
    else
    {
        get_be32(pb); /* creation time */
        get_be32(pb); /* modification time */
    }
    get_be32(pb); /* track id (NOT 0 !)*/
    get_be32(pb); /* reserved */
    //st->start_time = 0; /* check */
    (version == 1) ? get_be64(pb) : get_be32(pb); /* highlevel (considering edits) duration in movie timebase */

    FSKIP(pb, 16 + 36 + 8); /* display matrix */

    return 0;
}

static int32_t mov_read_mdhd(MOVContext *c, MOV_atom_t atom)
{
    ALOGV << "Reading \'mdhd\'" << std::endl;

    FILE *pb = c->fp;
    int32_t version = get_byte(pb);
    int32_t lang;

    if (version > 1)
        return 1; /* unsupported */

    get_byte(pb);
    get_byte(pb);
    get_byte(pb); /* flags */

    if (version == 1)
    {
        get_be64(pb);
        get_be64(pb);
    }
    else
    {
        get_be32(pb); /* creation time */
        get_be32(pb); /* modification time */
    }

    get_be32(pb);
    (version == 1) ? get_be64(pb) : get_be32(pb); /* duration */

    lang = get_be16(pb); /* language */
    get_be16(pb);        /* quality */

    return 0;
}

static int32_t mov_read_hdlr(MOVContext *c, MOV_atom_t atom)
{
    ALOGV << "Reading \'hdlr\'" << std::endl;

    uint32_t type;
    uint32_t ctype;
    FILE *pb = c->fp;

    get_byte(pb); /* version */
    get_byte(pb);
    get_byte(pb);
    get_byte(pb); /* flags */

    ctype = get_le32(pb);
    type = get_le32(pb); /* component subtype */

    if (!ctype)
        c->isom = 1;

    if (type == MKTAG('v', 'i', 'd', 'e'))
    {
        c->has_video = 1;
    }
    else if (type == MKTAG('s', 'o', 'u', 'n'))
    {
        c->has_audio = 1;
    }

    get_be32(pb); /* component  manufacture */
    get_be32(pb); /* component flags */
    get_be32(pb); /* component flags mask */

    if (atom.size <= 24)
        return 0; /* nothing left to read */

    FSKIP(pb, atom.size - (ftell(pb) - atom.offset));
    return 0;
}

static int32_t mov_read_cmov(MOVContext *c, MOV_atom_t atom)
{
    ALOGV << "Reading \'cmov\'" << std::endl;
    return -1; //Not support for compressed mode
}

static int mov_read_stco(MOVContext *c, MOV_atom_t atom)
{
    if (c->current_track_index < 0)
    {
        ALOGV << "STCO outside TRAK" << std::endl;
        return 0;
    }

    FILE *pb = c->fp;

    uint32_t xor_base = get_be32(pb);
    //get_byte(pb); /* version */
    //get_be24(pb); /* flags */

    // For ZRAW version and flags fields are used to store XOR base for values
    if (c->tracks_info.tracks[c->current_track_index].zraw_raw_version != 0x45A32DEF)
        xor_base = 0x0;

    uint32_t entries = get_be32(pb) ^ xor_base;
    if (!entries)
        return 0;

    // FIXME: Here must be check for duplicated 'stco'

    if (atom.type == MKTAG('s', 't', 'c', 'o'))
    {
        for (uint32_t i = 0; i < entries && !feof(pb); i++)
        {
            if (c->tracks_info.tracks[c->current_track_index].frames.size() < i + 1)
                c->tracks_info.tracks[c->current_track_index].frames.emplace_back();
            c->tracks_info.tracks[c->current_track_index].frames[i].frame_offset = get_be32(pb) ^ xor_base;
        }
    }
    else if (atom.type == MKTAG('c', 'o', '6', '4'))
        for (uint32_t i = 0; i < entries && !feof(pb); i++)
        {
            if (c->tracks_info.tracks[c->current_track_index].frames.size() < i + 1)
                c->tracks_info.tracks[c->current_track_index].frames.emplace_back();

            if (xor_base == 0)
                c->tracks_info.tracks[c->current_track_index].frames[i].frame_offset = get_be64(pb);
            else
                c->tracks_info.tracks[c->current_track_index].frames[i].frame_offset = get_be64(pb) ^ (0xFFFFFFFF00000000 | xor_base);
        }
    else
        return -1;

    if (feof(pb))
    {
        ALOGV << "Reached eof, corrupted STCO atom" << std::endl;
        return -1;
    }

    return 0;
}

static const uint32_t mac_to_unicode[128] = {
    0x00C4,
    0x00C5,
    0x00C7,
    0x00C9,
    0x00D1,
    0x00D6,
    0x00DC,
    0x00E1,
    0x00E0,
    0x00E2,
    0x00E4,
    0x00E3,
    0x00E5,
    0x00E7,
    0x00E9,
    0x00E8,
    0x00EA,
    0x00EB,
    0x00ED,
    0x00EC,
    0x00EE,
    0x00EF,
    0x00F1,
    0x00F3,
    0x00F2,
    0x00F4,
    0x00F6,
    0x00F5,
    0x00FA,
    0x00F9,
    0x00FB,
    0x00FC,
    0x2020,
    0x00B0,
    0x00A2,
    0x00A3,
    0x00A7,
    0x2022,
    0x00B6,
    0x00DF,
    0x00AE,
    0x00A9,
    0x2122,
    0x00B4,
    0x00A8,
    0x2260,
    0x00C6,
    0x00D8,
    0x221E,
    0x00B1,
    0x2264,
    0x2265,
    0x00A5,
    0x00B5,
    0x2202,
    0x2211,
    0x220F,
    0x03C0,
    0x222B,
    0x00AA,
    0x00BA,
    0x03A9,
    0x00E6,
    0x00F8,
    0x00BF,
    0x00A1,
    0x00AC,
    0x221A,
    0x0192,
    0x2248,
    0x2206,
    0x00AB,
    0x00BB,
    0x2026,
    0x00A0,
    0x00C0,
    0x00C3,
    0x00D5,
    0x0152,
    0x0153,
    0x2013,
    0x2014,
    0x201C,
    0x201D,
    0x2018,
    0x2019,
    0x00F7,
    0x25CA,
    0x00FF,
    0x0178,
    0x2044,
    0x20AC,
    0x2039,
    0x203A,
    0xFB01,
    0xFB02,
    0x2021,
    0x00B7,
    0x201A,
    0x201E,
    0x2030,
    0x00C2,
    0x00CA,
    0x00C1,
    0x00CB,
    0x00C8,
    0x00CD,
    0x00CE,
    0x00CF,
    0x00CC,
    0x00D3,
    0x00D4,
    0xF8FF,
    0x00D2,
    0x00DA,
    0x00DB,
    0x00D9,
    0x0131,
    0x02C6,
    0x02DC,
    0x00AF,
    0x02D8,
    0x02D9,
    0x02DA,
    0x00B8,
    0x02DD,
    0x02DB,
    0x02C7,
};

int ff_mov_read_stsd_entries(MOVContext *c, int entries)
{
    int pseudo_stream_id;

    //av_assert0(c->fc->nb_streams >= 1);

    FILE *pb = c->fp;

    for (pseudo_stream_id = 0; pseudo_stream_id < entries && !feof(pb); pseudo_stream_id++)
    {
        //Parsing Sample description table
        int ret, dref_id = 1;

        int64_t start_pos = FTELL(pb);

        int64_t size = get_be32(pb);    /* size */
        uint32_t format = get_le32(pb); /* data format */

        if (size >= 16)
        {
            get_be32(pb); /* reserved */
            get_be16(pb); /* reserved */
            dref_id = get_be16(pb);
        }
        else if (size <= 7)
        {
            ALOGV << "invalid size " << size << " in stsd" << std::endl;
            return -1;
        }

        char tmp[5] = {0};
        *(uint32_t *)&tmp = format;
        ALOGV << "stsd format: " << tmp << std::endl;
        c->tracks_info.tracks[c->current_track_index].codec_name = std::string(tmp);

        if (c->tracks_info.tracks[c->current_track_index].codec_name != "zraw")
        {
            // Here we skip all data that is not handled
            FSEEK(pb, start_pos + size);
            continue;
        }

        c->tracks_info.tracks[c->current_track_index].codec_type = TrackInfo::Video;

        int ver = get_be16(pb); // version
        int rev = get_be16(pb); // revision level
        int ven = get_be32(pb); // vendor
        int temp_q = get_be32(pb); // temporal quality
        int spac_q = get_be32(pb); // spatial quality
        int width = get_be16(pb); // width
        int height = get_be16(pb); // height

        c->tracks_info.tracks[c->current_track_index].width = width;
        c->tracks_info.tracks[c->current_track_index].height = height;

        // Next code is skipped because ZRAW codec does not use these fields
        //int hor_res = get_be32(pb); // horizontal resolution
        //int ver_res = get_be32(pb); // vertical resolution
        //int data_size = get_be32(pb); // data size (always 0)
        //int frames_per_sample = get_be16(pb); // frames per samples
        //uint8_t codec_name_len = get_byte(pb); // codec name length (<= 31)
        //FSKIP(pb, 31); // codec name
        //get_be16(pb); // depth
        //get_be16(pb); // colortable id

        FSKIP(pb, 46);

        get_be32(pb);   // SKIP
        uint32_t subatom_size = get_be32(pb);
        uint32_t subatom_name = get_be32(pb);

        if(subatom_name == 'zraw')
        {
            uint32_t raw_version = get_be32(pb); // raw_version (0x12EA78D2 or 0x45A32DEF)
            uint32_t unk0 = get_be32(pb); // unknown dword
            uint32_t unk1 = 0;

            if (subatom_size > 16)
            {
                unk1 = get_be32(pb);
            }

            ALOGV << "\'zraw\' track : [raw_version = 0x" << std::hex << raw_version \
                << " unk0 = " << std::dec << unk0 << " unk1 = " << unk1 << "]" << std::endl;

            c->tracks_info.tracks[c->current_track_index].zraw_raw_version = raw_version;
            c->tracks_info.tracks[c->current_track_index].zraw_unk0 = unk0;
            c->tracks_info.tracks[c->current_track_index].zraw_unk1 = unk1;
        }

        // Skip all that is not read
        FSEEK(pb, start_pos + size);
    }

    if (feof(pb))
    {
        ALOGV << "Reached eof, corrupted STSD atom" << std::endl;
        return -1;
    }

    return 0;
}

static int mov_read_stsd(MOVContext *c, MOV_atom_t atom)
{
    int ret;

    if (c->tracks_info.tracks.size() < 1)
        return 0;

    FILE *pb = c->fp;

    get_byte(pb); /* stsd_version */
    get_be24(pb); /* flags */

    int entries = get_be32(pb);

    /* Each entry contains a size (4 bytes) and format (4 bytes). */
    if (entries <= 0 || entries > atom.size / 8)
    {
        ALOGV << "invalid STSD entries " << entries << std::endl;
        return -1;
    }

    return ff_mov_read_stsd_entries(c, entries);
}

static int mov_read_free(MOVContext *c, MOV_atom_t atom)
{
    return 0;
}

static int mov_read_stsz(MOVContext *c, MOV_atom_t atom)
{
    unsigned int i, entries, sample_size, field_size, num_bytes;
    int ret;

    FILE *pb = c->fp;

    if (c->current_track_index < 0)
        return 0;

    get_byte(pb); /* version */
    get_be24(pb); /* flags */

    sample_size = get_be32(pb);

    field_size = 32;

    entries = get_be32(pb);

    ALOGV << "sample_size = " << sample_size << " sample_count = " << entries << std::endl;

    c->tracks_info.tracks[c->current_track_index].universal_sample_size = sample_size > 0 ? sample_size : -1;
    if (sample_size)
        return 0;

    if (field_size != 4 && field_size != 8 && field_size != 16 && field_size != 32)
    {
        ALOGV << "Invalid sample field size %u" << field_size << std::endl;
        return -1;
    }

    if (!entries)
        return 0;

    if (entries >= (UINT_MAX - 4) / field_size)
        return -1;

    for (int i = 0; i < entries; ++i)
    {
        if (feof(pb))
        {
            ALOGV << "Reached eof, corrupted STSZ atom" << std::endl;
            return -1;
        }

        // Read sample size
        const uint64_t sample_size_local = get_be32(pb);

        // Add frame if not exists yet
        if (c->tracks_info.tracks[c->current_track_index].frames.size() < i + 1)
            c->tracks_info.tracks[c->current_track_index].frames.emplace_back();

        // Save frame (sample) size
        c->tracks_info.tracks[c->current_track_index].frames[i].frame_size = sample_size_local;
    }

    if (feof(pb))
    {
        ALOGV << "Reached eof, corrupted STSZ atom" << std::endl;
        return -1;
    }

    return 0;
}

static const MOVParseTableEntry mov_default_parse_table[] =
{
    {MKTAG('e', 'd', 't', 's'), mov_read_default},
    {MKTAG('f', 't', 'y', 'p'), mov_read_ftyp},
    {MKTAG('h', 'd', 'l', 'r'), mov_read_hdlr},
    {MKTAG('m', 'd', 'a', 't'), mov_read_mdat},
    {MKTAG('m', 'd', 'h', 'd'), mov_read_mdhd},
    {MKTAG('m', 'd', 'i', 'a'), mov_read_default},
    {MKTAG('m', 'i', 'n', 'f'), mov_read_default},
    {MKTAG('m', 'o', 'o', 'v'), mov_read_moov},
    {MKTAG('m', 'v', 'h', 'd'), mov_read_mvhd},

    {MKTAG('s', 't', 'b', 'l'), mov_read_default},

    {MKTAG('s', 't', 'c', 'o'), mov_read_stco}, /* sample offsets */
    {MKTAG('c', 'o', '6', '4'), mov_read_stco}, /* sample offsets (64-bit) */

    {MKTAG('s', 't', 's', 'd'), mov_read_stsd}, /* sample info */
    {MKTAG('s', 't', 's', 'z'), mov_read_stsz}, /* sample sizes */

    {MKTAG('t', 'k', 'h', 'd'), mov_read_tkhd}, /* track header */
    {MKTAG('t', 'r', 'a', 'k'), mov_read_trak},

    {MKTAG('w', 'i', 'd', 'e'), mov_read_wide}, /* place holder */
    {MKTAG('c', 'm', 'o', 'v'), mov_read_cmov},

    {MKTAG('a', 'v', 'i', 'd'), mov_read_mdat},
    {MKTAG('3', 'd', 'v', 'f'), mov_read_moov},

    {MKTAG('f', 'r', 'e', 'e'), mov_read_free},
    {0L, NULL}
};

//--------------------------------------------------------

static int32_t mov_read_header(MOVContext *s, int content_size)
{
    int32_t err;
    MOV_atom_t atom = {0, 0, 0};
    FILE *pb;

    pb = s->fp;
    s->parse_table = mov_default_parse_table;

    if (content_size == 0)
        atom.size = fsize(pb);
    else
        atom.size = content_size;

    /* check MOV header */
    err = mov_read_default(s, atom);
    if (err < 0 || (!s->found_moov && !s->found_mdat))
    {
        return -1;
    }

    return 0;
}

int MovAudioOnlyDetect0(const char *url)
{
    MOVContext *c;

    c = (MOVContext *)malloc(sizeof(MOVContext));
    memset(c, 0, sizeof(MOVContext));

    c->fp = fopen(url, "rb");
    if (!c->fp)
    {
        free(c);
        return 0;
    }

    if (mov_read_header(c, 0) != 0)
        return 0;

    ALOGD << "has_audio: " << c->has_audio << " has_video: " << c->has_video << std::endl;

    fclose(c->fp);
    free(c);

    return (!c->has_video && c->has_audio);
}

TracksInfo_t MovDetectTracks(const char *url)
{
    MOVContext context;
    MOVContext *c = &context;

    c->fp = fopen(url, "rb");
    if (!c->fp)
        return c->tracks_info;

    if (mov_read_header(c, 0) != 0)
    {
        fclose(c->fp);
        c->tracks_info.output_log = c->string_stream.str();
        return c->tracks_info;
    }

    fclose(c->fp);
    c->tracks_info.output_log = c->string_stream.str();
    return c->tracks_info;
}

std::string MovDetectInfo(const char *url)
{
    MOVContext context;
    MOVContext *c = &context;

    c->fp = fopen(url, "rb");
    if (!c->fp)
    {
        return "Failed to open input file!";
    }

    if (mov_read_header(c, 0) != 0)
        return "Failed to read container info!";

    fclose(c->fp);
    
    c->string_stream << "MOV container detected!" << std::endl;
    for (int i = 0; i < c->tracks_info.tracks.size(); ++i)
        c->string_stream << "stream[" << i << "] = " << c->tracks_info.tracks[i].codec_name << std::endl;
    c->string_stream << "=======================" << std::endl;

    std::string res = c->string_stream.str();

    return res;
}

#include <fstream>
#include <string>
#include <vector>
#include <exception>

class ZrawMovRepresentation
{
public:
    ZrawMovRepresentation(std::string path, std::vector<TrackInfo_t> &tracks)
        : _filepath(path), _tracks(tracks)
    {
        // Save zraw tracks separately
        for (int i = 0; i < _tracks.size(); ++i)
            if (_tracks[i].codec_name == "zraw")
                _zraw_tracks.push_back(_tracks[i]);
    }

    int zrawStreamsCount()
    {
        return _zraw_tracks.size();
    }

    int zrawStreamFramesCount(uint32_t stream_index)
    {
        return _zraw_tracks[stream_index].frames.size();
    }

    std::vector<uint8_t> zrawFrame(uint32_t stream_index, uint32_t frame_index)
    {
        // Extract saved offset and size
        const auto offset = _zraw_tracks[stream_index].frames[frame_index].frame_offset;
        const auto size = _zraw_tracks[stream_index].frames[frame_index].frame_size;

        // Open file
        std::fstream file_in(_filepath, std::fstream::in | std::fstream::binary);
        if (!file_in.is_open())
        {
            throw new std::exception(); //"Could not open file!"
        }

        // Seek read position to zraw frame offset
        file_in.seekg(offset, file_in.beg);

        // Read frame
        std::vector<uint8_t> frame(size);
        file_in.read((char *)frame.data(), size);

        // Close file
        file_in.close();

        return frame;
    }

private:
    std::string _filepath;
    std::vector<TrackInfo_t> _tracks;
    std::vector<TrackInfo_t> _zraw_tracks;
};

class ZrawMovTrackRepresentation
{
public:
    ZrawMovTrackRepresentation(std::istream& f_in, TrackInfo_t &track)
        : _track(track), _f_in(f_in) {}
    
    int zrawStreamFramesCount(uint32_t stream_index)
    {
        return _track.frames.size();
    }

    std::vector<uint8_t> zrawFrame(uint32_t frame_index)
    {
        // Extract saved offset and size
        uint32_t offset = _track.frames[frame_index].frame_offset;
        uint32_t size = _track.frames[frame_index].frame_size;

        // Seek read position to zraw frame offset
        _f_in.seekg(offset, _f_in.beg);

        // Read frame
        std::vector<uint8_t> frame(size);
        _f_in.read((char *)frame.data(), size);

        return frame;
    }

private:
    std::istream& _f_in;
    TrackInfo_t& _track;
};