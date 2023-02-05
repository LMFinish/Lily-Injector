#ifndef DOLFILE_H_INCLUDED
#define DOLFILE_H_INCLUDED

    struct Dol_File {
    uint32_t Text_Physical[7];
    uint32_t Data_Physical[11];
    uint32_t Text_Virtual[7];
    uint32_t Data_Virtual[11];
    uint32_t Text_Lengths[7];
    uint32_t Data_Lengths[11];
    uint32_t BSS_Address;
    uint32_t BSS_Length;
    uint32_t Entry_Point;
} dol;

#endif // DOLFILE_H_INCLUDED
