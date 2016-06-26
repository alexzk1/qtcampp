//License: MIT, (c) Oleksiy Zakharov, 2016, alexzkhr@gmail.com

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <libv4l2.h>
#include <string.h>
#include <dirent.h>

#include "v4l2device.h"
#define FATAL_RISE(TEXT) throw v4l2device::v4l2device_excp(std::string(TEXT)+"\n\tat "+std::string(__FILE__)+": "+std::to_string(__LINE__))

#define WRAPPER_RET(FUNC, arg...) if (usingWrapper) return ::v4l2_##FUNC(arg); else return ::FUNC(arg)

int v4l2device::fd() const
{
    if (!m_fd)
        return -1;

    return m_fd->get();
}

v4l2device::v4l2device()
{
    memset(&m_capability, 0, sizeof(m_capability));
}

v4l2device::~v4l2device()
{
    close();
}

v4l2device::devices_list_t v4l2device::list_attached_devices(v4l2device::ListType type)
{
    devices_list_t res;
    res.reserve(64); //v4l says 64 is max

    const std::string static base_dir = "/dev";

    auto dirp = opendir(base_dir.c_str());
    if (dirp)
    {
        v4l2device tmp;
        for(auto dp = readdir(dirp); dp; dp = readdir(dirp))
        {
            std::string name (dp->d_name);
            if (type == ListType::VIDEO && name.find("video", 0) == 0)
            {
                device_info inf;
                inf.sys_path = base_dir + "/" + name;
                if (tmp.open(inf.sys_path))
                {
                    v4l2_capability cap;
                    tmp.querycap(cap);
                    tmp.close();
                    inf.devname = std::string(reinterpret_cast<const char*>(cap.card));
                    inf.devCaps = cap.device_caps;

                    res.push_back(inf);
                }
            }
        }
        (void)closedir(dirp);
    }
    return res;
}

v4l2device::device_controls v4l2device::listControls() const
{
    device_controls result;
    v4l2_query_ext_ctrl qctrl;

    qctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL | V4L2_CTRL_FLAG_NEXT_COMPOUND;
    while(queryctrl(qctrl))
    {
        if (!(qctrl.flags & V4L2_CTRL_FLAG_DISABLED))
            result.push_back(qctrl);
        qctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL | V4L2_CTRL_FLAG_NEXT_COMPOUND;
    }
    return result;
}

v4l2device::device_control_menu v4l2device::listMenuForControl(const v4l2_query_ext_ctrl &control) const
{
    device_control_menu menu;
    if (control.type == V4L2_CTRL_TYPE_MENU || control.type == V4L2_CTRL_TYPE_INTEGER_MENU)
    {
        menu_item qmenu;
        for (qmenu.menu.index = 0; qmenu.menu.index <= control.maximum; ++qmenu.menu.index)
        {
            qmenu.menu.id = control.id;
            if (querymenu(qmenu.menu))
            {

                qmenu.isInteger = control.type == V4L2_CTRL_TYPE_INTEGER_MENU;
                menu.push_back(qmenu);
            }
        }
    }
    return menu;
}

bool v4l2device::open(const std::string &device, bool tryWraper)
{
    close();
    usingWrapper = false;
    auto fd = ::open(device.c_str(), O_RDWR | O_NONBLOCK);
    if (fd < 0)
        return false; //FATAL_RISE("Cannot open " + device);

    memset(&m_capability, 0, sizeof(m_capability));
    if (::ioctl(fd, VIDIOC_QUERYCAP, &m_capability) < 0)
    {
        ::close(fd);
        return false;
        //FATAL_RISE(device + " is not a V4L2 device.");
    }

    if (tryWraper)
    {
        auto fd2 = ::v4l2_fd_open(fd, V4L2_ENABLE_ENUM_FMT_EMULATION);
        if (fd2 > -1)
        {
            fd = fd2;
            usingWrapper = true;
        }
        //else fallback to regular i/o
    }

    m_device = device;
    m_fd.reset(new dev_hndl(fd, [this](int f)
    {
        WRAPPER_RET(close, f);
    }));
    return true;
}

bool v4l2device::reopen()
{
    std::string currName(reinterpret_cast<const char*>(m_capability.card));
    std::string d = m_device;
    auto ca = m_capability;
    bool w = usingWrapper;

    auto r = open(d, usingWrapper);

    if (r && currName != std::string(reinterpret_cast<const char*>(m_capability.card)))
    {
       close(); //opened something, but it's other device
       r = false;
    }

    //that's because close() wipes data, and we may need to call reopen() later again
    m_device     = d;
    usingWrapper = w;
    m_capability = ca;
    return r;
}

void v4l2device::close()
{
    m_fd.reset();
    m_device = "";
    usingWrapper = false;
    memset(&m_capability, 0, sizeof(m_capability));
}

bool v4l2device::is_valid_yet()
{
    int f = fd();
    if (f == -1)
        return false;

    struct stat s;
    fstat(f, &s);

    return s.st_nlink > 0;
}

int v4l2device::setControlValue(const v4l2_query_ext_ctrl &control, __s64 value) const
{
    v4l2_ext_control c;
    c.id = control.id;

    if (!(control.flags & V4L2_CTRL_FLAG_HAS_PAYLOAD))
    {
        if ((control.type & V4L2_CTRL_TYPE_INTEGER64))
            c.value64 = value;
        else
            c.value = static_cast<__s32>(value);

        v4l2_ext_controls arr;
        arr.ctrl_class = 0; //may fail here if driver does not support it
        //arr.which = V4L2_CTRL_WHICH_CUR_VAL; //...or here, its union

        arr.count = 1;
        memset(&arr.reserved, 0, sizeof(arr.reserved));
        arr.controls = &c;

        return ioctl(VIDIOC_S_EXT_CTRLS, &arr);
    }

    return EINVAL;
}

__u32 v4l2device::queryControlFlags(__u32 id)
{
   v4l2_query_ext_ctrl c;
   c.id = id;
   if (queryctrl(c))
       return c.flags;
   return 0xFFFFFFFF; // all flags set - fail
}

bool v4l2device::querycap(v4l2_capability &cap) const
{
    memset(&cap, 0, sizeof(cap));
    return ioctl(VIDIOC_QUERYCAP, &cap) >= 0;
}

bool v4l2device::queryctrl(v4l2_query_ext_ctrl &qc) const
{
    memset(qc.reserved, 0 , sizeof(qc.reserved));
    return ioctl(VIDIOC_QUERY_EXT_CTRL, &qc) >= 0;
}

bool v4l2device::querymenu(v4l2_querymenu &qm) const
{
    qm.reserved = 0;
    return ioctl(VIDIOC_QUERYMENU, &qm) >= 0;
}

int v4l2device::ioctl(v4l2device::ioctlr_t cmd, void *arg) const
{
    WRAPPER_RET(ioctl, fd(), cmd, arg);
}

ssize_t v4l2device::read(uint8_t *p, size_t size) const
{
    WRAPPER_RET(read, fd(), p, size);
}


v4l2device::v4l2device_excp::~v4l2device_excp()
{
    //keeping VIRTUAL destructor in cpp file so virtual table will be created once
}

v4l2device_ptr v4l2device::device_info::open(bool wrapper) const
{
    v4l2device_ptr r(new v4l2device());
    if (!r->open(sys_path, wrapper))
        r.reset();
    return r;
}
