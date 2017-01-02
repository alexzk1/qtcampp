#ifndef V4L2DEVICE_H
#define V4L2DEVICE_H

/*
 * (C) By Oleksiy Zakharov 2016, alexzkhr@gmail.com
 *
 * Inspired by "v4l2-api: low-level wrapper around v4l2 devices"
 *
 * That is kept minimal to solve 1 problem - record video only from webcam, feel free to add more things.
 * License: MIT
*/
#include <string>
#include <vector>
#include <memory>
#include <exception>
#include <thread>
#include <atomic>
#include <functional>
#include <stdint.h>
#include "auto_closable.h"
#include "video_camera.h"

#include <linux/videodev2.h>

class v4l2device : public video_camera
{
public:
    class v4l2device_excp : public std::runtime_error
    {
    public:
        explicit v4l2device_excp(const std::string& t): std::runtime_error(t){}
        explicit v4l2device_excp(const char* t): std::runtime_error(t){}
        virtual ~v4l2device_excp();
    };

    enum class ListType
    {
        VIDEO,
    };

    struct device_info
    {
        std::string sys_path;
        std::string devname;
        uint32_t    devCaps;
        device_info():
            sys_path(),
            devname(),
            devCaps(0)
        {}

        video_camera_ptr open(bool wrapper = false) const;
    };

    //unlike v4l menu this stores flag itself if item is text or numeric
    struct menu_item
    {
        v4l2_querymenu menu;
        bool           isInteger; //field value must be used instead caption if true
    };

    struct mmapped_buffer
    {
        bool usingWrapper;
        v4l2_buffer buf;
        uint8_t*  memory;
        size_t mem_len;

        mmapped_buffer(const mmapped_buffer&) = delete;
        mmapped_buffer& operator = (const mmapped_buffer&) = delete;

        mmapped_buffer(bool usingWrapper);
        ~mmapped_buffer();
        int do_unmap();
        uint8_t *do_map(int fd);
    };

    using ioctlr_t            = unsigned long int;

    using devices_list_t      = std::vector<device_info>;
    using device_controls     = std::vector<v4l2_query_ext_ctrl>;
    using device_control_menu = std::vector<menu_item>;
    using mmapped_buffer_ptr  = std::shared_ptr<mmapped_buffer>;

    using device_formats_t    = std::vector<v4l2_fmtdesc>;
    using buffers_t           = std::vector<mmapped_buffer_ptr>;
protected:
    using dev_hndl   = auto_closable<int>;
    using dev_hndl_p = std::shared_ptr<dev_hndl>;
private:
    dev_hndl_p  m_fd;
    std::string m_device;
    bool usingWrapper;
    v4l2_capability m_capability;
    std::atomic<bool> sourcePixelFormatChanged;
    void inputCameraFunction(const utility::runnerint_t& should_stop);
protected:
    int     fd() const;
    bool    querycap(v4l2_capability &cap) const;
    bool    queryctrl(v4l2_query_ext_ctrl &qc) const;
    bool    querymenu(v4l2_querymenu &qm) const;
    int     ioctl(ioctlr_t cmd, void *arg) const;
    ssize_t read(uint8_t *p, size_t size) const;

    int     reqbuffs(__u32 count, __u32 type = V4L2_BUF_TYPE_VIDEO_CAPTURE, __u32 memory = V4L2_MEMORY_MMAP) const;
    int     query_buf( v4l2_buffer *buf) const;

    int     create_buffers(buffers_t& buffers) const;
    int     free_buffers  (buffers_t& buffers) const;


    bool    streamon(__u32 buftype);
    bool    streamoff(__u32 buftype);

    bool    get_fmt_cap(__u32 type, v4l2_format &fmt) const;
    int     set_fmt_cap(v4l2_format &fmt) const;

    //bool    enum_frame_sizes() const;
    virtual utility::runner_f_t getInputFunction() override;
public:
    v4l2device();
    v4l2device(const v4l2device&) = delete;
    v4l2device& operator= (const v4l2device&) = delete;
    virtual ~v4l2device();

    //device's controls
    devices_list_t static list_attached_devices(ListType type = ListType::VIDEO);
    device_controls     listControls() const;
    device_control_menu listMenuForControl(const v4l2_query_ext_ctrl& control) const;
    int setControlValue(const v4l2_query_ext_ctrl& control,__s64 value) const; //todo: overload for string
    __u32 queryControlFlags(__u32 id);

    device_formats_t listFormats(__u32 type = V4L2_BUF_TYPE_VIDEO_CAPTURE);
    int  setSourcePixelFormat( __u32 frm, __u32 type = V4L2_BUF_TYPE_VIDEO_CAPTURE);
    bool open(const std::string& device, bool tryWraper);

    //device's state (opened, closed)
    virtual bool open(const std::string& device) override;
    virtual bool reopen()override; //tries reopen
    virtual void close()override;
    virtual bool is_valid_yet() const override; //tests if device is connected/opened yet

};

#endif // V4L2DEVICE_H
