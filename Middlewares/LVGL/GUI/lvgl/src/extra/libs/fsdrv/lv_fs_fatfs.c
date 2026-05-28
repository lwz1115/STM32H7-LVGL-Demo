/**
 * @file lv_fs_fatfs.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "../../../lvgl.h"

#if LV_USE_FS_FATFS
#include "ff.h"
#include <stdio.h>
/*********************
 *      DEFINES
 *********************/

#if LV_FS_FATFS_LETTER == '\0'
    #error "LV_FS_FATFS_LETTER must be an upper case ASCII letter"
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void fs_init(void);

static void * fs_open(lv_fs_drv_t * drv, const char * path, lv_fs_mode_t mode);
static lv_fs_res_t fs_close(lv_fs_drv_t * drv, void * file_p);
static lv_fs_res_t fs_read(lv_fs_drv_t * drv, void * file_p, void * buf, uint32_t btr, uint32_t * br);
static lv_fs_res_t fs_write(lv_fs_drv_t * drv, void * file_p, const void * buf, uint32_t btw, uint32_t * bw);
static lv_fs_res_t fs_seek(lv_fs_drv_t * drv, void * file_p, uint32_t pos, lv_fs_whence_t whence);
static lv_fs_res_t fs_tell(lv_fs_drv_t * drv, void * file_p, uint32_t * pos_p);
static void * fs_dir_open(lv_fs_drv_t * drv, const char * path);
static lv_fs_res_t fs_dir_read(lv_fs_drv_t * drv, void * dir_p, char * fn);
static lv_fs_res_t fs_dir_close(lv_fs_drv_t * drv, void * dir_p);

/**********************
 *  STATIC VARIABLES
 **********************/
/* fs_sd 已移除：FatFS 挂载由 main.c 统一管理，LVGL 文件系统直接复用 */

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_fs_fatfs_init(void)
{
    /*----------------------------------------------------
     * Initialize your storage device and File System
     * -------------------------------------------------*/
    fs_init();

    /*---------------------------------------------------
     * Register the file system interface in LVGL
     *--------------------------------------------------*/

    /*Add a simple drive to open images*/
    static lv_fs_drv_t fs_drv; /*A driver descriptor*/
    lv_fs_drv_init(&fs_drv);

    /*Set up fields...*/
    fs_drv.letter = LV_FS_FATFS_LETTER;
    fs_drv.cache_size = LV_FS_FATFS_CACHE_SIZE;

    fs_drv.open_cb = fs_open;
    fs_drv.close_cb = fs_close;
    fs_drv.read_cb = fs_read;
    fs_drv.write_cb = fs_write;
    fs_drv.seek_cb = fs_seek;
    fs_drv.tell_cb = fs_tell;

    fs_drv.dir_close_cb = fs_dir_close;
    fs_drv.dir_open_cb = fs_dir_open;
    fs_drv.dir_read_cb = fs_dir_read;

    lv_fs_drv_register(&fs_drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief       ��ʼ���洢�豸���ļ�ϵͳ
 * @param       ��
 * @retval      ��
 */
static void fs_init(void)
{
    /* SD卡和FatFS已在main.c中初始化并挂载到"S:"
     * 这里不需要重复初始化，LVGL文件系统直接复用已挂载的FatFS实例 */
}

/**
 * @brief ��һ���ļ�
 * @param drv��ָ��ú�����������������
 * @param path������������ͷ���ļ�·��(����S:/folder/file.txt)
 * @param mode: FS_MODE_RD, write: FS_MODE_WR, both: FS_MODE_RD | FS_MODE_WR
 * @retval ��NULL: �ɹ�, NULLP��ʧ��
 */
static void * fs_open(lv_fs_drv_t * drv, const char * path, lv_fs_mode_t mode)
{
    LV_UNUSED(drv);
    uint8_t flags = 0;

    if(mode == LV_FS_MODE_WR) flags = FA_WRITE | FA_OPEN_ALWAYS;
    else if(mode == LV_FS_MODE_RD) flags = FA_READ;
    else if(mode == (LV_FS_MODE_WR | LV_FS_MODE_RD)) flags = FA_READ | FA_WRITE | FA_OPEN_ALWAYS;

    FIL * f = lv_mem_alloc(sizeof(FIL));
    if(f == NULL) return NULL;

    /* LVGL 传入的 path 已去掉驱动字母（如 "S:"），只剩 "/simhei_16.bin"
     * FatFs 需要完整路径，拼回 "S:" 前缀 */
    char full_path[256];
    if(path[0] == '/') {
        /* path = "/xxx" → "S:/xxx" */
        snprintf(full_path, sizeof(full_path), "%c:%s", LV_FS_FATFS_LETTER, path);
    } else {
        snprintf(full_path, sizeof(full_path), "%c:/%s", LV_FS_FATFS_LETTER, path);
    }

    FRESULT res = f_open(f, full_path, flags);
    if(res == FR_OK) {
        return f;
    }
    else {
        printf("[lv_fs] f_open failed: %s (err=%d)\r\n", full_path, res);
        lv_mem_free(f);
        return NULL;
    }
}

 /**
 * @brief  �ر��Ѵ򿪵��ļ�
 * @param  drv��ָ��ú�����������������
 * @param  file_p��ָ��file_t������ָ�롣(��lv_ufs_open��)
 * @retval LV_FS_RES_OK:û�д����ļ�����ȡ����lv_fs_res_t enum���κδ���
 */
static lv_fs_res_t fs_close(lv_fs_drv_t * drv, void * file_p)
{
    LV_UNUSED(drv);
    f_close(file_p);
    lv_mem_free(file_p);
    return LV_FS_RES_OK;
}

/**
 * @brief �Ӵ򿪵��ļ��ж�ȡ����
 * @param drv��ָ��ú�����������������
 * @param file_p��ָ��file_t������ָ�롣
 * @param buf��ָ�룬ָ��洢�����ݵ��ڴ��
 * @param btr��Ҫ��ȡ���ֽ���
 * @param br��ʵ�ʶ��ֽ���(�ֽڶ�)
 * @retval LV_FS_RES_OK:û�д����ļ�����ȡ�κδ�������lv_fs_res_t enum
 */
static lv_fs_res_t fs_read(lv_fs_drv_t * drv, void * file_p, void * buf, uint32_t btr, uint32_t * br)
{
    LV_UNUSED(drv);
    FRESULT res = f_read(file_p, buf, btr, (UINT *)br);
    if(res == FR_OK) return LV_FS_RES_OK;
    else return LV_FS_RES_UNKNOWN;
}

/**
 * @brief  д���ļ�
 * @param  drv��   ָ��ú�����������������
 * @param  file_p��ָ��file_t������ָ��
 * @param  buf��   ָ�룬ָ��һ������Ҫд���ֽڵĻ�����
 * @param  btw��   Ҫд��btr�ֽ���
 * @param  br��    ʵ��д����ֽ���(��д����ֽ���)�����δʹ��NULL��
 * @retval LV_FS_RES_OK:û�д����ļ�����ȡ�κδ�������lv_fs_res_t enum
 */
static lv_fs_res_t fs_write(lv_fs_drv_t * drv, void * file_p, const void * buf, uint32_t btw, uint32_t * bw)
{
    LV_UNUSED(drv);
    FRESULT res = f_write(file_p, buf, btw, (UINT *)bw);
    if(res == FR_OK) return LV_FS_RES_OK;
    else return LV_FS_RES_UNKNOWN;
}

/**
 * @brief  ���ö�дָ�롣����б�Ҫ��Ҳ������չ�ļ���С��
 * @param  drv��   ָ��ú�����������������
 * @param  file_p��ָ��file_t������ָ�롣(ʹ��lv_ufs_open��)
 * @param  pos��   ��дָ�����λ��
 * @retval LV_FS_RES_OK:û�д����ļ�����ȡ�κδ�������lv_fs_res_t enum
 */
static lv_fs_res_t fs_seek(lv_fs_drv_t * drv, void * file_p, uint32_t pos, lv_fs_whence_t whence)
{
    LV_UNUSED(drv);
    switch(whence) {
        case LV_FS_SEEK_SET:
            f_lseek(file_p, pos);
            break;
        case LV_FS_SEEK_CUR:
            f_lseek(file_p, f_tell((FIL *)file_p) + pos);
            break;
        case LV_FS_SEEK_END:
            f_lseek(file_p, f_size((FIL *)file_p) + pos);
            break;
        default:
            break;
    }
    return LV_FS_RES_OK;
}

/**
 * @brief  ���ض�дָ���λ��
 * @param  drv��   ָ��ú�����������������
 * @param  file_p��ָ��file_t������ָ��
 * @param  pos_p�� ���ڴ洢�����ָ��
 * @retval LV_FS_RES_OK:û�д����ļ�����ȡ�κδ�������lv_fs_res_t enum
 */
static lv_fs_res_t fs_tell(lv_fs_drv_t * drv, void * file_p, uint32_t * pos_p)
{
    LV_UNUSED(drv);
    *pos_p = f_tell((FIL *)file_p);
    return LV_FS_RES_OK;
}

/**
 * @brief  ��Ŀ¼
 * @param  drv��    ָ��ú�����������������
 * @param  rddir_p��ָ��'lv_fs_dir_t'������ָ��
 * @param  path��   Ŀ¼·��
 * @retval ָ���ʼ����'DIR'������ָ��
 */
static void * fs_dir_open(lv_fs_drv_t * drv, const char * path)
{
    LV_UNUSED(drv);
    DIR * d = lv_mem_alloc(sizeof(DIR));
    if(d == NULL) return NULL;

    /* 同 fs_open，拼回驱动字母前缀 */
    char full_path[256];
    if(path[0] == '/') {
        snprintf(full_path, sizeof(full_path), "%c:%s", LV_FS_FATFS_LETTER, path);
    } else {
        snprintf(full_path, sizeof(full_path), "%c:/%s", LV_FS_FATFS_LETTER, path);
    }

    FRESULT res = f_opendir(d, full_path);
    if(res != FR_OK) {
        lv_mem_free(d);
        d = NULL;
    }
    return d;
}

/**
 * @brief  ��һ��Ŀ¼�ж�ȡ��һ���ļ���
 * @param  drv��    ָ��ú�����������������
 * @param  rddir_p��ָ���ʼ���ġ�lv_fs_dir_t��������ָ��
 * @param  fn��     ָ�����ļ����Ļ�������ָ��
 * @retval LV_FS_RES_OK:û�д����ļ�����ȡ�κδ�������lv_fs_res_t enum
 */
static lv_fs_res_t fs_dir_read(lv_fs_drv_t * drv, void * dir_p, char * fn)
{
    LV_UNUSED(drv);
    FRESULT res;
    FILINFO fno;
    fn[0] = '\0';

    do {
        res = f_readdir(dir_p, &fno);
        if(res != FR_OK) return LV_FS_RES_UNKNOWN;

        if(fno.fattrib & AM_DIR) {
            fn[0] = '/';
            strcpy(&fn[1], fno.fname);
        }
        else strcpy(fn, fno.fname);

    } while(strcmp(fn, "/.") == 0 || strcmp(fn, "/..") == 0);

    return LV_FS_RES_OK;
}

/**
 * @brief  �ر�Ŀ¼��ȡ
 * @param  drv��    ָ��ú�����������������
 * @param  rddir_p��ָ���ʼ���ġ�lv_fs_dir_t��������ָ��
 * @retval LV_FS_RES_OK:û�д����ļ�����ȡ�κδ�������lv_fs_res_t enum
 */
static lv_fs_res_t fs_dir_close(lv_fs_drv_t * drv, void * dir_p)
{
    LV_UNUSED(drv);
    f_closedir(dir_p);
    lv_mem_free(dir_p);
    return LV_FS_RES_OK;
}

#else /*LV_USE_FS_FATFS == 0*/

#if defined(LV_FS_FATFS_LETTER) && LV_FS_FATFS_LETTER != '\0'
    #warning "LV_USE_FS_FATFS is not enabled but LV_FS_FATFS_LETTER is set"
#endif

#endif /*LV_USE_FS_POSIX*/

