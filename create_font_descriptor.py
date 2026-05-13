import struct
import os

def create_golden_font_descriptor(path):
    print(f"Creating Golden Font Descriptor: {path}")
    
    name = "Font"
    basename = "font"
    pattern = "#?.(font|ttf|otf|otag|type1)"
    code = "Recognizer"
    
    # FourCCs
    GID_PICTURE = int.from_bytes(b'pict', 'big')
    ID_FONT     = int.from_bytes(b'font', 'big')

    def pad_str(s):
        b = s.encode() + b'\0'
        if len(b) % 2:
            b += b'\0'
        return b

    # Helper to build chunks
    def chunk(cid, data):
        if len(data) % 2:
            data += b'\0'
        return cid + struct.pack(">I", len(data)) + data

    # Prepare strings
    s_name = pad_str(name)
    s_base = pad_str(basename)
    s_patt = pad_str(pattern)
    s_code = pad_str(code)
    s_ver  = pad_str("$VER: Font 54.1 (13.05.2026) [Golden Path]")

    # Calculate offsets for DTHD
    off_name = 32
    off_base = off_name + len(s_name)
    off_patt = off_base + len(s_base)
    
    # DTHD body
    dthd_header = struct.pack(">III hh II hh I", 
        off_name,
        off_base,
        off_patt,
        127,      # Priority (MAXIMUM)
        0,        # Flags
        GID_PICTURE,
        ID_FONT,
        0,        # MaskLen
        0,        # Pad
        0         # Extra Pad
    )
    dthd_body = dthd_header + s_name + s_base + s_patt

    # Build FORM DTYP
    form_data = b"DTYP"
    form_data += chunk(b"FVER", s_ver)
    form_data += chunk(b"NAME", s_name)
    form_data += chunk(b"DTHD", dthd_body)
    
    # DTCD Chunk - Standalone Recognizer Code
    form_data += chunk(b"DTCD", s_code)
    
    # DTMK Chunk - Optional Mask for speed
    dtmk_body = struct.pack(">Q I HH", 0, 12, 0xFFFC, 0x0F00)
    form_data += chunk(b"DTMK", dtmk_body)

    # Wrap in IFF FORM
    full_data = b"FORM" + struct.pack(">I", len(form_data)) + form_data
    
    with open(path, "wb") as f:
        f.write(full_data)
    with open(path.lower(), "wb") as f:
        f.write(full_data)
        
    print(f"Golden Descriptor created successfully. Size: {len(full_data)} bytes.")
    print(f"Code: {code}, Base: {basename}")

create_golden_font_descriptor("/opt/code/AmigaOS4-datatypes/font_dt/Font")
