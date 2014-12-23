#include <ppu-lv2.h>
#include <stdio.h>
#include <string.h>

#define NAND_FLASH_DEV_ID		0x100000000000001ULL
#define NAND_FLASH_START_SECTOR		0x204
#define NOR_FLASH_DEV_ID		0x100000000000004ULL
#define NOR_FLASH_START_SECTOR		0x178

#define FLASH_SECTOR_SIZE		0x200
#define READ_FLAGS			0x22


static inline int get_ps_id (char *v) {
  lv2syscall1 (872, (u64)v);
  return_to_user_prog(s32);
}

static inline int get_console_id (char *v) {
  lv2syscall1 (870, (u64)v);
  return_to_user_prog(s32);
}

static inline int lv2_storage_open(uint64_t dev_id, uint32_t *dev_handle)
{
  lv2syscall4(600, dev_id, 0, (uint64_t) dev_handle, 0);
  return_to_user_prog(int);
}

static inline int lv2_storage_close(uint32_t dev_handle)
{
  lv2syscall1(601, dev_handle);
  return_to_user_prog(int);
}

static inline int lv2_storage_read(uint32_t dev_handle, uint64_t unknown1,
    uint64_t start_sector, uint64_t sector_count,
    const void *buf, uint32_t *unknown2, uint64_t flags)
{
  lv2syscall7(602, dev_handle, unknown1, start_sector, sector_count,
      (uint64_t ) buf, (uint64_t) unknown2, flags);
  return_to_user_prog(int);
}


s32 get_idp_psid (char *idp, char *psid) {
  uint64_t start_sector;
  uint8_t buffer[FLASH_SECTOR_SIZE];
  uint32_t read;
  uint32_t dev_handle = 0;
  int result;

  memset (idp, 0, 16);
  memset (psid, 0, 16);
  memset (buffer, 0, FLASH_SECTOR_SIZE);

  get_console_id (idp);
  result = get_ps_id (psid);

  if (memcmp (idp, buffer, 16) == 0) {
    result = lv2_storage_open(NOR_FLASH_DEV_ID, &dev_handle);
    if (result) {
      start_sector = NAND_FLASH_START_SECTOR;
      lv2_storage_close(dev_handle);
      result = lv2_storage_open(NAND_FLASH_DEV_ID, &dev_handle);
    } else {
      start_sector = NOR_FLASH_START_SECTOR;
    }
    if (result == 0) {
      result = lv2_storage_read(dev_handle, 0, start_sector, 1,
          buffer, &read, READ_FLAGS);
    }
    lv2_storage_close(dev_handle);

    memcpy (idp, buffer + 112, 16);
  }

  return result;
}

int main () {
  char idp[16];
  char psid[16];
  char filename[1024];
  s32 ret;
  int i;
  FILE *f;

  if (get_idp_psid (idp, psid) != 0)
    return -1;

  for (i = 0; i < 10; i++) {
    snprintf (filename, 1024, "/dev_usb%03d/idps.bin", i);
    f = fopen (filename, "wb");
    printf ("Opening file %s : %p\n", filename, f);
    if (f)
      break;
  }
  if (!f)
    return -1;

  fwrite (idp, 16, 1, f);
  fclose (f);

  snprintf (filename, 1024, "/dev_usb%03d/psid.bin", i);
  f = fopen (filename, "wb");
  if (!f)
    return -1;
  fwrite (psid, 16, 1, f);
  fclose (f);

  return 0;
}
