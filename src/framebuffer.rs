use font8x8;
use mmio;

const PROPERTY_CHANNEL: usize = 8;

#[allow(dead_code)] const FRAMEBUFFER_CHANNEL: usize = 1;

const CHAR_WIDTH:  u32 = 8;
const CHAR_HEIGHT: u32 = 8;

// Current property mailbox status
struct MailStatus {
    // Fields: -
    //  - [0..29]: reserved
    //  - 30: read buffer empty flag
    //  - 31: write buffer full flag
    bitfield: u32,
}
impl MailStatus {
    fn new() -> MailStatus {
        MailStatus { bitfield: 0 }
    }

    pub fn is_read_empty(&self) -> bool {
        (self.bitfield & (1 << 30)) > 0
    }

    pub fn is_write_full(&self) -> bool {
        (self.bitfield & (1 << 31)) > 0
    }

    pub fn update(&mut self) {
        self.bitfield = mmio::Mmio::read(mmio::GPU_MAILBOX_STATUS);
    }
}

// A single property mailbox message which can be read from/written to a channel
struct MailMessage {
    // Fields: -
    //  - [0..3]:  channel ID
    //  - [4..31]: message data
    bitfield: u32,
}
impl MailMessage {
    fn new() -> MailMessage {
        MailMessage { bitfield: 0 }
    }

    pub fn get_channel_id(&self) -> usize {
        (self.bitfield & 0x0F) as usize
    }

    pub fn set_channel_id(&mut self, id: u8) {
        self.bitfield = (self.bitfield & 0xFFFFFFF0) | ((id & 0x0F) as u32)
    }

    pub fn get_data(&self) -> usize {
        ((self.bitfield & 0xFFFFFFF0) >> 4) as usize
    }

    pub fn set_data(&mut self, data: u32) {
        self.bitfield = (self.bitfield & 0x0F) | (((data << 4) & 0xFFFFFFF0) as u32)
    }

    pub fn update(&mut self) {
        self.bitfield = mmio::Mmio::read(mmio::GPU_MAILBOX_READ);
    }

    pub fn write(&self) {
        mmio::Mmio::write(mmio::GPU_MAILBOX_WRITE, self.bitfield);
    }

    pub fn mailbox_read(channel: usize) -> MailMessage {
        let mut counter:   usize =     0;
        let     max_count: usize = 20000;
        let mut status:    MailStatus  = MailStatus::new();
        let mut res:       MailMessage = MailMessage::new();

        // Loop until the channel read matches the requested channel
        loop {

            // Loop until the mailbox status shows not empty
            loop {
                status.update();
                counter += 1;
                if counter >= max_count || !status.is_read_empty() {
                    break;
                }
            }

            // Read from mailbox
            res.update();

            if res.get_channel_id() == channel || counter >= max_count {
                break;
            }
        }

        res
    }

    pub fn mailbox_write(&mut self, channel: usize) {
        let mut status: MailStatus = MailStatus::new();
        self.set_channel_id(channel as u8);

        // Loop until the mailbox status shows not full
        loop {
            status.update();
            if !status.is_write_full() {
                break;
            }
        }

        // Write message to mailbox
        self.write();
    }
}

// Result type returned from FrameBuffer24::init() and ::alloc()
pub enum FBInitResult {
    Success,
    RequestNotProcessed,
    ResponseError,
}

// Struct to encapsulate a sigle framebuffer and all associated framebuffer functionality
#[derive(Debug)]
pub struct FrameBuffer24 {
    pub width:        u32,
    pub height:       u32,
    pub bpp:          u32,
    pub pitch:        u32,
    pub buf:          *mut u8,
    pub size:         u32,
    pub chars_width:  u32,
    pub chars_height: u32,
    pub x:            u32,
    pub y:            u32,
}
impl FrameBuffer24 {
    pub fn new(width: u32, height: u32) -> Result<FrameBuffer24, ()> {
        let mut fb = FrameBuffer24 {
            width:        width,
            height:       height,
            bpp:          24,
            pitch:        0,
            buf:          0 as *mut u8,
            size:         0,
            chars_width:  0,
            chars_height: 0,
            x:            0,
            y:            0,
        };

        match fb.init() {
            FBInitResult::RequestNotProcessed => return Err(()),
            FBInitResult::ResponseError       => return Err(()),
            _ => {},
        };

        match fb.alloc() {
            FBInitResult::RequestNotProcessed => return Err(()),
            FBInitResult::ResponseError       => return Err(()),
            _ => {},
        };

        Ok(fb)
    }

    fn init(&mut self) -> FBInitResult {
        let mut mail: MailMessage = MailMessage::new();
        let mut req_init: FBInitMessage = FBInitMessage {
            size:  0,
            mtype: REQUEST,

            tags: [

                // Tag 0: set physical dimensions (size: 20)
                MessageTag {
                    proptag: FB_SET_PHYSICAL_DIMENSIONS,
                    size:    2 * 4,
                    mtype:   REQUEST,
                    value_buffer: ValueBuffer { fb_screen_size: FBScreenSize { width: self.width, height: self.height } },
                },

                // Tag 1: set virtual dimensions (size: 20)
                MessageTag {
                    proptag: FB_SET_VIRTUAL_DIMENSIONS,
                    size:    2 * 4,
                    mtype:   REQUEST,
                    value_buffer: ValueBuffer { fb_screen_size: FBScreenSize { width: self.width, height: self.height } },
                },

                // Tag 2: set bit depth (size: 16)
                MessageTag {
                    proptag: FB_SET_BITS_PER_PIXEL,
                    size:    4,
                    mtype:   REQUEST,
                    value_buffer: ValueBuffer { fb_bits_per_pixel: self.bpp },
                },
            ],

            end_tag: NULL_TAG as u32,
        };

        // Calculate message size (must be padded to 16 byte alignment)
        req_init.size = 3 * 4 + (req_init.tags[0].size + req_init.tags[1].size + req_init.tags[2].size) + (9 * 4);
        req_init.size += if req_init.size % 16 > 0 { 16 - (req_init.size % 16) } else { 0 };

        // Low 4 bits of address are 0 as address is 16 byte aligned and "data" must be highest 28 bits
        // (32 - 4), so shift right by 4
        mail.set_data((((&req_init as *const _) as u32) >> 4) as u32);
        mail.mailbox_write(PROPERTY_CHANNEL);

        let mail = MailMessage::mailbox_read(PROPERTY_CHANNEL);

        // After writing and reading, the MailMessage will contain the address of the FBInitMessage
        // that is filled out by the GPU. Therefore, access this struct as a raw pointer to get the
        // result.
        let res_init: *mut FBInitMessage = (mail.get_data() << 4) as *mut FBInitMessage;

        unsafe {
            if (*res_init).mtype == REQUEST {
                return FBInitResult::RequestNotProcessed;
            } else if (*res_init).mtype == RESPONSE_ERROR || (*res_init).mtype != RESPONSE_SUCCESS {
                return FBInitResult::ResponseError;
            }
        }

        // Fill in all possible attributes so far
        self.chars_width  = self.width  / CHAR_WIDTH;
        self.chars_height = self.height / CHAR_HEIGHT;
        self.x = 0;
        self.y = 0;
        self.pitch = self.width * (self.bpp / 8);

        FBInitResult::Success
    }

    fn alloc(&mut self) -> FBInitResult {
        let mut mail: MailMessage = MailMessage::new();
        let mut req_alloc: FBAllocMessage = FBAllocMessage {
            size:  0,
            mtype: REQUEST,
            tag: MessageTag {
                proptag: FB_ALLOCATE_BUFFER,
                size:    2 * 4,
                mtype:   REQUEST,
                value_buffer: { ValueBuffer { fb_allocate_align: 16 } },
            },
            end_tag: NULL_TAG as u32,
        };
        req_alloc.size = 6 * 4 + req_alloc.tag.size;
        req_alloc.size += if req_alloc.size % 16 > 0 { 16 - (req_alloc.size % 16) } else { 0 };

        mail.set_data((((&req_alloc as *const _) as u32) >> 4) as u32);
        mail.mailbox_write(PROPERTY_CHANNEL);
        let mail = MailMessage::mailbox_read(PROPERTY_CHANNEL);

        let res_alloc: *mut FBAllocMessage = (mail.get_data() << 4) as *mut FBAllocMessage;

        unsafe {
            if (*res_alloc).mtype == REQUEST {
                return FBInitResult::RequestNotProcessed;
            } else if (*res_alloc).mtype == RESPONSE_ERROR || (*res_alloc).mtype != RESPONSE_SUCCESS {
                return FBInitResult::ResponseError;
            }

            self.buf  = (*res_alloc).tag.value_buffer.fb_allocate_res.fb_addr;
            self.size = (*res_alloc).tag.value_buffer.fb_allocate_res.fb_size;
        }

        FBInitResult::Success
    }

    pub fn draw_test_pattern(&self) {
        unsafe {
            for y in 0..self.height {
                let off_y = y * self.pitch;
                for x in 0..self.width {
                    let off = off_y + (x * 3);
                    let p: Pixel24 = Pixel24 {
                        r: x as u8 + y as u8,
                        g: x as u8 - y as u8,
                        b: x as u8 * y as u8,
                    };
                    *(self.buf.offset(off as isize) as *mut Pixel24) = p;
                }
            }
        }
    }

    pub fn putpixel(&self, x: u32, y: u32, p: &Pixel24) {
        if x > self.width - 1 {
            return;
        }
        if y > self.height - 1 {
            return;
        }
        unsafe {
            *(self.buf.offset((y * self.pitch + x * 3) as isize) as *mut Pixel24) = p.clone();
        }
    }

    pub fn putchar(&self, ch: char, posx: u32, posy: u32, col: &Pixel24) {
        if ch as u8 >= 0x80 {
            return;
        }

        let black: Pixel24 = Pixel24 {r: 0, g: 0, b: 0};

        // Draw character at (posx,posy)
        for y in 0..8 {
            for x in 0..8 {
                let row: u8 = font8x8::CHARS[ch as usize][y];
                self.putpixel(posx + x as u32, posy + y as u32, if row & (0x80 >> x) > 0 { col } else { &black });
            }
        }
    }

    pub fn scroll_y(&mut self, pixels: u32) {
        let black: Pixel24 = Pixel24 {r: 0, g: 0, b: 0};

        // Scroll current pixels
        unsafe {
            for y in pixels..self.height {
                for x in 0..self.width {
                    let src = self.buf.offset(( y           * self.pitch + (x * 3)) as isize) as *const Pixel24;
                    let dst = self.buf.offset(((y - pixels) * self.pitch + (x * 3)) as isize) as *mut   Pixel24;
                    *dst = (*src).clone();
                }
            }
        }

        // Fill new pixels with black
        unsafe {
            for y in (self.height - pixels)..self.height {
                for x in 0..self.width {
                    let dst = self.buf.offset((y * self.pitch + (x * 3)) as isize) as *mut Pixel24;
                    *dst = black.clone();
                }
            }
        }
    }

    fn handle_scroll(&mut self) {
      if self.y >= self.chars_height {
        let diff = self.y - self.chars_height + 1;
        self.scroll_y(diff * CHAR_HEIGHT);
        self.y -= diff;
      }
    }

    pub fn writechar(&mut self, ch: char, col: &Pixel24) {
      let offx: u32 = self.x * CHAR_WIDTH;
      let offy: u32 = self.y * CHAR_HEIGHT;
      let black: Pixel24 = Pixel24 {r: 0, g: 0, b: 0};

      // Carriage return
      if ch == '\n' || ch == '\r' {
        self.putcursor(&black);
        self.x  = 0;
        self.y += 1;
        self.handle_scroll();
        return;
      }

      // Backspace
      if ch == '\x08' {
        if self.x == 0 {
          self.x = self.chars_width - 1;
          self.y -= 1;
        } else {
          self.x -= 1;
        }
        return;
      }

      self.putchar(ch, offx, offy, col);

      // Increment FB cursor position
      self.x += 1;
      if self.x >= self.chars_width {
        self.x  = 0;
        self.y += 1;
        self.handle_scroll();
      }
    }

    pub fn putcursor(&self, col: &Pixel24) {
        for y in 0..8 {
            for x in 0..8 {
                self.putpixel(self.x * CHAR_WIDTH + x, self.y * CHAR_HEIGHT + y, col);
            }
        }
    }

    pub fn write_string(&mut self, s: &str, col: &Pixel24) {
        for ch in s.as_bytes() {
            self.writechar(ch.clone() as char, col);
        }
    }
}


// Property mailbox tag types
const NULL_TAG:                   u32 = 0x0;
const FB_ALLOCATE_BUFFER:         u32 = 0x00040001;
const FB_SET_PHYSICAL_DIMENSIONS: u32 = 0x00048003;
const FB_SET_VIRTUAL_DIMENSIONS:  u32 = 0x00048004;
const FB_SET_BITS_PER_PIXEL:      u32 = 0x00048005;

#[allow(dead_code)] const FB_RELEASE_BUFFER:          u32 = 0x00048001;
#[allow(dead_code)] const FB_GET_PHYSICAL_DIMENSIONS: u32 = 0x00040003;
#[allow(dead_code)] const FB_GET_VIRTUAL_DIMENSIONS:  u32 = 0x00040004;
#[allow(dead_code)] const FB_GET_BITS_PER_PIXEL:      u32 = 0x00040005;
#[allow(dead_code)] const FB_GET_BYTES_PER_ROW:       u32 = 0x00040008;

// Property mailbox request/response types
const REQUEST:          u32 = 0x0;
const RESPONSE_SUCCESS: u32 = 0x80000000;
const RESPONSE_ERROR:   u32 = 0x80000001;

// Framebuffer allocation data (for FB_ALLOCATE_BUFFER)
#[derive(Copy, Clone)]
struct FBAllocateRes {
    fb_addr: *mut u8, // Address of FB start
    fb_size: u32,     // Total size of FB
}

// Framebuffer size data (for FB_(G|S)ET_(PHYSICAL|VIRTUAL)_DIMENSIONS)
#[allow(dead_code)]
#[derive(Copy, Clone)]
struct FBScreenSize {
    width:  u32,
    height: u32,
}

// A MessageTag value buffer can any one of these types at a time
union ValueBuffer {
    fb_allocate_align: u32,
    fb_allocate_res:   FBAllocateRes,
    fb_screen_size:    FBScreenSize,
    fb_bits_per_pixel: u32,
}

// A property mailbox message contains one or more MessageTag
#[allow(dead_code)]
struct MessageTag {
    proptag:      u32,         // Tag type
    size:         u32,         // Size of the value buffer (bytes)
    mtype:        u32,         // Type: request or response
    value_buffer: ValueBuffer, // Tag value buffer
}

// Message to send to the property mailbox in order to initialise a framebuffer
#[repr(C)]
#[repr(align(16))]
struct FBInitMessage {
    size:    u32,             // Size of buffer, including "size"
    mtype:   u32,             // Request/response type
    tags:    [MessageTag; 3], // Tags for the message
    end_tag: u32,             // NULL tag
}

// Message to send to the property mailbox in order to allocate a previously initialised
// framebuffer
#[repr(C)]
#[repr(align(16))]
struct FBAllocMessage {
    size:    u32,        // Size of buffer, including "size"
    mtype:   u32,        // Request/response and response type
    tag:     MessageTag, // Only Tag for the message
    end_tag: u32,        // NULL tag
}

#[derive(Clone)]
pub struct Pixel24 {
    pub r: u8,
    pub g: u8,
    pub b: u8,
}
