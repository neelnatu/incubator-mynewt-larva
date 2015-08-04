#include <assert.h>
#include <string.h>
#include "ffs_priv.h"
#include "ffs/ffs.h"

int
ffs_area_desc_validate(const struct ffs_area_desc *area_desc)
{
    return 0;
}

static void
ffs_area_set_magic(struct ffs_disk_area *disk_area)
{
    disk_area->fda_magic[0] = FFS_AREA_MAGIC0;
    disk_area->fda_magic[1] = FFS_AREA_MAGIC1;
    disk_area->fda_magic[2] = FFS_AREA_MAGIC2;
    disk_area->fda_magic[3] = FFS_AREA_MAGIC3;
}

int
ffs_area_magic_is_set(const struct ffs_disk_area *disk_area)
{
    return disk_area->fda_magic[0] == FFS_AREA_MAGIC0 &&
           disk_area->fda_magic[1] == FFS_AREA_MAGIC1 &&
           disk_area->fda_magic[2] == FFS_AREA_MAGIC2 &&
           disk_area->fda_magic[3] == FFS_AREA_MAGIC3;
}

int
ffs_area_is_scratch(const struct ffs_disk_area *disk_area)
{
    return ffs_area_magic_is_set(disk_area) &&
           disk_area->fda_id == FFS_AREA_ID_NONE;
}

void
ffs_area_to_disk(struct ffs_disk_area *out_disk_area,
                 const struct ffs_area *area)
{
    memset(out_disk_area, 0, sizeof *out_disk_area);
    ffs_area_set_magic(out_disk_area);
    out_disk_area->fda_length = area->fa_length;
    out_disk_area->fda_ver = FFS_AREA_VER;
    out_disk_area->fda_gc_seq = area->fa_gc_seq;
    out_disk_area->fda_id = area->fa_id;
}

uint32_t
ffs_area_free_space(const struct ffs_area *area)
{
    return area->fa_length - area->fa_cur;
}

int
ffs_area_find_corrupt_scratch(uint16_t *out_good_idx, uint16_t *out_bad_idx)
{
    const struct ffs_area *iarea;
    const struct ffs_area *jarea;
    int i;
    int j;

    for (i = 0; i < ffs_num_areas; i++) {
        iarea = ffs_areas + i;
        for (j = i + 1; j < ffs_num_areas; j++) {
            jarea = ffs_areas + j;

            if (jarea->fa_id == iarea->fa_id) {
                /* Found a duplicate.  The shorter of the two areas should be
                 * used as scratch.
                 */
                if (iarea->fa_cur < jarea->fa_cur) {
                    *out_good_idx = j;
                    *out_bad_idx = i;
                } else {
                    *out_good_idx = i;
                    *out_bad_idx = j;
                }

                return 0;
            }
        }
    }

    return FFS_ECORRUPT;
}