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
#include <linux/videodev2.h>
#include <string>
#include <vector>
#include <memory>
#include <exception>
#include <stdint.h>
#include "auto_closable.h"

class v4l2device;
using v4l2device_ptr = std::shared_ptr<v4l2device>;

class v4l2device
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

        v4l2device_ptr open(bool wrapper = false) const;
    };

    //unlike v4l menu this stores flag itself if item is text or numeric
    struct menu_item
    {
        v4l2_querymenu menu;
        bool           isInteger; //field value must be used instead caption if true
    };

    using devices_list_t      = std::vector<device_info>;
    using device_controls     = std::vector<v4l2_query_ext_ctrl>;
    using device_control_menu = std::vector<menu_item>;
protected:
    using dev_hndl   = auto_closable<int>;
    using dev_hndl_p = std::shared_ptr<dev_hndl>;
public:
    using ioctlr_t   = unsigned long int;
private:
    dev_hndl_p  m_fd;
    std::string m_device;
    bool usingWrapper;
    v4l2_capability m_capability;

protected:
    int     fd() const;
    bool    querycap(v4l2_capability &cap) const;
    bool    queryctrl(v4l2_query_ext_ctrl &qc) const;
    bool    querymenu(v4l2_querymenu &qm) const;
    int     ioctl(ioctlr_t cmd, void *arg) const;
    ssize_t read(uint8_t *p, size_t size) const;
public:
    v4l2device();
    v4l2device(const v4l2device&) = delete;
    v4l2device& operator= (const v4l2device&) = delete;
    virtual ~v4l2device();

    devices_list_t static list_attached_devices(ListType type = ListType::VIDEO);

    device_controls     listControls() const;
    device_control_menu listMenuForControl(const v4l2_query_ext_ctrl& control) const;

    bool open(const std::string& device, bool tryWraper = false);
    bool reopen(); //tries reopen
    void close();
    bool is_valid_yet(); //tests if device is connected/opened yet

    int setControlValue(const v4l2_query_ext_ctrl& control,__s64 value) const; //todo: overload for string
    __u32 queryControlFlags(__u32 id);
};




#endif // V4L2DEVICE_H
