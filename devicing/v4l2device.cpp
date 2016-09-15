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
#include <thread>
#include <chrono>
#include <iostream>

#include "v4l2device.h"



const static int lostDevicePauseMs    = 150; //sleep of the thread when device is lost/does errors
const static uint64_t deviceTestEachNLoops = 1000; //not too often check if we didnt miss device yet

#ifdef QTCAMPP_PROJ
//some integration with qtcampp, accessing qt stuffs etc...
#pragma message ("Enabled QTCAMPP")
#include "ui/globalsettings.h"
#define BUFFERS_AMOUNT (StaticSettingsMap::getGlobalSetts().readInt("VideoBuffs"))
#else
//standalone, stl solution, maybe used elsewhere
#define BUFFERS_AMOUNT 10
#endif

#define FATAL_RISE(TEXT) throw v4l2device::v4l2device_excp(std::string(TEXT)+"\n\tat "+std::string(__FILE__)+": "+std::to_string(__LINE__))
#define WRAPPER_RET(FUNC, arg...) if (usingWrapper) return ::v4l2_##FUNC(arg); else return ::FUNC(arg)
#define WRAPPER_NORET(FUNC, arg...) if (usingWrapper) ::v4l2_##FUNC(arg); else ::FUNC(arg)


int v4l2device::fd() const
{
    if (!m_fd)
        return -1;

    return m_fd->get();
}

v4l2device::v4l2device():
    interruptor(true),
    m_thread(nullptr),
    sourcePixelFormatChanged(false)
{
    memset(&m_capability, 0, sizeof(m_capability));
}

v4l2device::~v4l2device()
{
    stopCameraInput();
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
    if (device.empty())
        return false;

    usingWrapper = false;
    auto fd = ::open(device.c_str(), O_RDWR | O_NONBLOCK);
    if (fd < 0)
    {
        std::cerr<<"Failed to open "<<device.c_str()<<", code: "<<errno<<std::endl;
        return false; //FATAL_RISE("Cannot open " + device);
    }

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
#ifdef _DEBUG
    std::cerr<<"Opened FD: "<<fd<<std::endl;
#endif
    m_device = device;
    m_fd.reset(new dev_hndl(fd, [this](int f)
    {
#ifdef _DEBUG
        std::cerr<<"Closing FD: "<<f<<std::endl;
#endif
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

v4l2device::device_formats_t v4l2device::listFormats(__u32 type)
{
    device_formats_t res;
    res.reserve(10); //hope 10 formats will be enough in most cases
    v4l2_fmtdesc fmt;
    memset(&fmt, 0, sizeof(v4l2_fmtdesc));
    fmt.type = type;

    for (__u32 index = 0;; ++index)
    {
        fmt.index = index;
        auto code = ioctl(VIDIOC_ENUM_FMT, &fmt);

        if (code < 0)
            break;
        res.push_back(fmt);
    }

    return res;
}

bool v4l2device::startCameraInput()
{

    bool res = false;
    if (is_valid_yet() && !m_thread)
    {
        m_thread.reset(new std::thread([this]()
        {
            //thread function
            bool lostDevice = true;
            buffers_t buffers;
            v4l2_buffer cam_buf;
            v4l2_format srcFormat;

            //cleansing structs, guess driver will change only some bits
            memset(&srcFormat,  0, sizeof(v4l2_format));
            memset(&cam_buf,    0, sizeof(v4l2_buffer));


            //default values
            cam_buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            cam_buf.memory = V4L2_MEMORY_MMAP;
            uint64_t loopCounter = 0;

            //device maybe busy, especially at the startup + i7 cpu, so need to wait some if driver says so
            //not really sure what will happen if cable will be disconnected exatly inside loop
            auto busy_wait = [this](__u32 code, v4l2_buffer& cam_buf) ->int
            {
                int co = 0;
                while(interruptor)
                {
                    co = ioctl(code, &cam_buf);

                    if (co == -1)
                        co = errno;
                    else
                        break;

                    if (co == EAGAIN)
                        std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(10));
                    else
                        break;
                }
                return co;
            };


            while (interruptor)
            {

                if (lostDevice)
                {
                    if (!(lostDevice = create_buffers(buffers) < 1))
                    {
                        //initConverter
                        sourcePixelFormatChanged = false;
                        for (auto& b : buffers) //enqueue all buffers
                            busy_wait(VIDIOC_QBUF, b->buf);
                        cam_buf.type   = buffers.at(0)->buf.type;
                        cam_buf.memory = buffers.at(0)->buf.memory;
                        get_fmt_cap(cam_buf.type, srcFormat);

                        lostDevice = true;
                        namedListeners.for_each([this, &srcFormat, &lostDevice](listeners_holder_t::ForEachT& p)
                        {
                            if (p.second)
                            {
                                p.second->initConverter(fd(), srcFormat);
                                lostDevice &= !p.second->isInitialized();
                            }
                        });
                        if (!lostDevice)
                            streamon(cam_buf.type);
                    }
                }

                if (!lostDevice)
                {

                    int code = busy_wait(VIDIOC_DQBUF, cam_buf);
                    lostDevice = (code < 0);

                    if (!lostDevice && code != EAGAIN)
                    {
                        const auto& buf = buffers.at(cam_buf.index);

                        if (!(cam_buf.flags & V4L2_BUF_FLAG_ERROR))
                        {
                            namedListeners.for_each([this, &srcFormat, &buf, &lostDevice](listeners_holder_t::ForEachT& p)
                            {
                                if (p.second)
                                {
                                    try
                                    {
                                        //trying to init converter if it was added while everything is running already
                                        if (!p.second->isInitialized())
                                        {
                                            p.second->initConverter(fd(), srcFormat);
                                        }
                                        p.second->setNextFrame(srcFormat, buf->memory, buf->mem_len);
                                    }
                                    catch (std::exception& e)
                                    {
                                        std::cerr << "Exception in call to supply data: " << e.what()<<std::endl;
                                        lostDevice = true;
                                    }
                                }
                            });
                        }

                        cam_buf.flags = cam_buf.reserved = 0;
                        code = busy_wait(VIDIOC_QBUF, cam_buf);
                        lostDevice = (code < 0);
                    }
                }

                //not too often check if we didnt miss device yet
                if (++loopCounter % deviceTestEachNLoops == 0)
                    lostDevice = lostDevice || !is_valid_yet();

                if (lostDevice || sourcePixelFormatChanged)
                {
                    //lets close full subsystem, maybe it will be long to re-init it back, but ...

                    streamoff(cam_buf.type);
                    free_buffers(buffers);
                    if (lostDevice)
                    {
                        std::cerr << "Lost device ...sleeping, restarting all." <<std::endl;
                        std::this_thread::sleep_for(std::chrono::duration<decltype (lostDevicePauseMs), std::milli>(lostDevicePauseMs));
                    }
                }
                lostDevice |= sourcePixelFormatChanged;
            }
            streamoff(cam_buf.type);
            free_buffers(buffers);

        }), [this](std::thread *p)
        {
            //thread deletor, ensuring it will be stopped
            interruptor = false;
            if (p)
            {
                if (p->joinable())
                    p->join();
                delete p;
            }
        });
    }
    return res;
}

int v4l2device::setSourcePixelFormat(__u32 frm, __u32 type)
{
    //ok not sure, my camera has 1 format only, so even adding fake test changes nothing
    //maybe driver just ignores set request
    v4l2_format src;
    memset(&src,  0, sizeof(v4l2_format));
    get_fmt_cap(type, src);
    src.fmt.pix.pixelformat = frm;
    int r = EBUSY;
    for(int i =0; i < 30 && r == EBUSY; ++i)
    {
        r = set_fmt_cap(src);
        if (r == EBUSY)
            std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(10));
    }
    sourcePixelFormatChanged = r > -1;
    return r;
}

void v4l2device::stopCameraInput()
{
    m_thread.reset();
}

bool v4l2device::isCameraRunning()
{
    return m_thread != nullptr;
}

void v4l2device::setNamedListener(const std::string &name, const frame_listener_ptr &listener)
{
    namedListeners.set(name, listener);
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

int v4l2device::reqbuffs(__u32 count, __u32 type, __u32 memory) const
{
    v4l2_requestbuffers r;
    r.count = count;
    r.memory = memory;
    r.type = type;
    memset(r.reserved, 0, sizeof(r.reserved));
    if (ioctl(VIDIOC_REQBUFS, &r) > -1)
        return static_cast<int>(r.count);
    return -1;
}

int v4l2device::query_buf(v4l2_buffer *buf) const
{
    buf->reserved  = 0;
    buf->reserved2 = 0;
    return ioctl(VIDIOC_QUERYBUF, buf);
}

int v4l2device::create_buffers(buffers_t &buffers) const
{
    const static auto type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    const static auto memory = V4L2_MEMORY_MMAP;
    auto bamount = static_cast<__u32>(BUFFERS_AMOUNT);
    buffers.reserve(bamount);
    buffers.clear();
    int count = reqbuffs(bamount, type, memory);
    if (count > 0)
    {
        for (int i = 0; i < count; i++)
        {
            mmapped_buffer_ptr ptr(new mmapped_buffer(usingWrapper));
            ptr->buf.index = static_cast<__u32>(i);
            ptr->buf.type = type;
            ptr->buf.memory = memory;

            if (query_buf(&ptr->buf) > -1)
            {
                if (ptr->do_map(fd()))
                {
                    buffers.push_back(ptr);
                }
            }
        }
        return static_cast<int>(buffers.size());
    }
    return -1;
}

int v4l2device::free_buffers(buffers_t &buffers) const
{
    buffers.clear();
    return reqbuffs(0);
}

bool v4l2device::streamon(__u32 buftype)
{
    return ioctl(VIDIOC_STREAMON, &buftype);
}

bool v4l2device::streamoff(__u32 buftype)
{
    return ioctl(VIDIOC_STREAMOFF, &buftype);
}

bool v4l2device::get_fmt_cap(__u32 type, v4l2_format &fmt) const
{
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = type;
    return ioctl(VIDIOC_G_FMT, &fmt) > -1;
}

int v4l2device::set_fmt_cap(v4l2_format &fmt) const
{
    if (V4L2_TYPE_IS_MULTIPLANAR(fmt.type))
    {
        fmt.fmt.pix_mp.plane_fmt[0].bytesperline = 0;
        fmt.fmt.pix_mp.plane_fmt[1].bytesperline = 0;
    }
    else
        fmt.fmt.pix.bytesperline = 0;

    return ioctl(VIDIOC_S_FMT, &fmt);
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

v4l2device::mmapped_buffer::~mmapped_buffer()
{
    do_unmap();
}

v4l2device::mmapped_buffer::mmapped_buffer(bool usingWrapper):
    usingWrapper(usingWrapper),
    memory(nullptr)
{
    buf.memory = V4L2_MEMORY_MMAP;
    buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
}

int v4l2device::mmapped_buffer::do_unmap()
{
    if ((buf.flags & V4L2_BUF_FLAG_MAPPED) && memory)
    {
        auto m = memory;
        memory = nullptr;
        WRAPPER_RET(munmap, m, mem_len);
    }
    return  -1;
}

uint8_t *v4l2device::mmapped_buffer::do_map(int fd)
{
    mem_len = buf.length; //maybe will use some extended features later, so will copy actual value to mem_len
    if(MAP_FAILED == (memory = static_cast<decltype(memory)>(mmap(0, mem_len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, buf.m.offset))))
        memory = nullptr;
    return memory;
}
