# Imports.  {  ###
import os
import struct
from enum import Enum, unique
from typing import BinaryIO, NamedTuple

# }


def parse(filename):
    # {  ###
    """parses a .ase file and returns a list of colors and color groups

    `swatch.parse` reads in an ase file and converts it to a list of colors and
    palettes. colors are simple dicts of the form

    ```json
    {
        'name': u'color name',
        'type': u'Process',
        'data': {
            'mode': u'RGB',
            'values': [1.0, 1.0, 1.0]
        }
    }
    ```

    the values provided vary between color mode. For all color modes, the
    value is always a list of floats.

    RGB: three floats between [0,1]  corresponding to RGB.
    CMYK: four floats between [0,1] inclusive, corresponding to CMYK.
    Gray: one float between [0,1] with 1 being white, 0 being black.
    LAB: three floats. The first L, is ranged from 0,1. Both A and B are
    floats ranging from [-128.0,127.0]. I believe illustrator just crops
    these to whole values, though.

    Palettes (née Color Groups in Adobe Parlance) are also dicts, but they have an
    attribute named `swatches` which contains a list of colors contained within
    the palette.

    ```json
    {
        'name': u'accent colors',
        'type': u'Color Group',
        'swatches': [
            {color}, {color}, ..., {color}
        ]
    }
    ```

    Because Adobe Illustrator lets swatches exist either inside and outside
    of palettes, the output of swatch.parse is a list that may contain
    swatches and palettes, i.e. [ swatch* palette* ]

    Here's an example with a light grey swatch followed by a color group containing three

        >>> import swatch
        >>> swatch.parse("example.ase")
        [{'data': {'mode': u'Gray', 'values': [0.75]},
          'name': u'Light Grey',
          'type': u'Process'},
         {'name': u'Accent Colors',
          'swatches': [{'data': {'mode': u'CMYK',
             'values': [0.5279774069786072,
              0.24386966228485107,
              1.0,
              0.04303044080734253]},
            'name': u'Green',
            'type': u'Process'},
           {'data': {'mode': u'CMYK',
             'values': [0.6261844635009766,
              0.5890134572982788,
              3.051804378628731e-05,
              3.051804378628731e-05]},
            'name': u'Violet Process Global',
            'type': u'Global'},
           {'data': {'mode': u'LAB', 'values': [0.6000000238418579, -35.0, -5.0]},
            'name': u'Cyan Spot (global)',
            'type': u'Spot'}],
          'type': u'Color Group'}]
    """

    with open(filename, "rb") as data:
        header, v_major, v_minor, _ = struct.unpack("!4sHHI", data.read(12))

        assert header == b"ASEF"
        assert (v_major, v_minor) == (1, 0)

        return list(_parse_chunk(data))
    # }


def write(obj, filename):
    # {  ###
    """write a swatch object to the filename specified.

    if `filename` exists, it will be overwritten.

    `obj` *must* be a list of swatches and palettes, as follows

    ```
        [ swatch* palette* ]
    ```

    the best source for how each of these are described is in the `parser`
    documentation.
    """
    with open(filename, "wb") as f:
        header = b"ASEF"
        v_major, v_minor = 1, 0
        chunk_count = _chunk_count(obj)
        head = struct.pack("!4sHHI", header, v_major, v_minor, chunk_count)
        body = b"".join([_chunk_for_object(c) for c in obj])
        f.write(head + body)
        # }


def _parse_chunk(fd):
    # {  ###
    chunk_type = fd.read(2)
    while chunk_type:
        if chunk_type == b"\x00\x01":
            # a single color
            o = _dict_for_chunk(fd)
            yield o

        elif chunk_type == b"\xc0\x01":
            # folder/palate
            o = _dict_for_chunk(fd)
            o["swatches"] = list(_colors(fd))
            yield o

        elif chunk_type == b"\xc0\x02":
            # this signals the end of a folder
            assert fd.read(4) == b"\x00\x00\x00\x00"

        else:
            # the file is malformed?
            assert chunk_type in [b"\xc0\x01", b"\x00\x01", b"\xc0\x02", b"\x00\x02"]

        chunk_type = fd.read(2)
    # }


def _colors(fd):
    # {  ###
    chunk_type = fd.read(2)
    while chunk_type in [b"\x00\x01", b"\x00\x02"]:
        d = _dict_for_chunk(fd)
        yield d
        chunk_type = fd.read(2)
    fd.seek(-2, os.SEEK_CUR)
    # }


def _dict_for_chunk(fd):
    # {  ###
    chunk_length = struct.unpack(">I", fd.read(4))[0]
    data = fd.read(chunk_length)

    title_length = (struct.unpack(">H", data[:2])[0]) * 2
    title = data[2 : 2 + title_length].decode("utf-16be").strip("\0")
    color_data = data[2 + title_length :]

    output = {
        "name": str(title),
        "type": "Color Group",  # default to color group
    }

    if color_data:
        fmt = {b"RGB": "!fff", b"Gray": "!f", b"CMYK": "!ffff", b"LAB": "!fff"}
        color_mode = struct.unpack("!4s", color_data[:4])[0].strip()
        color_values = list(struct.unpack(fmt[color_mode], color_data[4:-2]))

        color_types = ["Global", "Spot", "Process"]
        swatch_type_index = struct.unpack(">h", color_data[-2:])[0]
        swatch_type = color_types[swatch_type_index]

        output.update(
            {
                "data": {"mode": color_mode.decode("utf-8"), "values": color_values},  # type: ignore[dict-item]
                "type": str(swatch_type),
            }
        )

    return output
    # }


def _chunk_count(swatch):
    # {  ###
    """return the number of byte-chunks in a swatch object

    this recursively walks the swatch list, returning 1 for a single color &
    returns 2 for each folder plus 1 for each color it contains
    """
    if type(swatch) is dict:
        if "data" in swatch:
            return 1
        if "swatches" in swatch:
            return 2 + len(swatch["swatches"])

        assert False
    else:
        return sum(map(_chunk_count, swatch))

    return -1
    # }


def _chunk_for_object(obj):
    # {  ###
    type_ = obj.get("type")
    if type_ == "Color Group":
        return _chunk_for_folder(obj)
    if type_ in ["Process", "Spot", "Global"]:
        return _chunk_for_color(obj)

    assert False
    return None
    # }


def _chunk_for_color(obj):
    # {  ###
    """builds up a byte-chunk for a color

    the format for this is
        b'\x00\x01' +
        Big-Endian Unsigned Int == len(bytes that follow in this block)
          • Big-Endian Unsigned Short == len(color_name)
              in practice, because utf-16 takes up 2 bytes per letter
              this will be 2 * (len(name) + 1)
              so a color named 'foo' would be 8 bytes long
          • UTF-16BE Encoded color_name terminated with '\0'
              using 'foo', this yields '\x00f\x00o\x00o\x00\x00'
          • A 4-byte char for Color mode ('RGB ', 'Gray', 'CMYK', 'LAB ')
              note the trailing spaces
          • a variable-length number of 4-byte length floats
              this depends entirely on the color mode of the color.
          • A Big-Endian short int for either a global, spot, or process color
              global == 0, spot == 1, process == 2

    the chunk has no terminating string although other sites have indicated
    that the global/spot/process short is a terminator, it's actually used
    to indicate how illustrator should deal with the color.
    """
    title = obj["name"] + "\0"
    title_length = len(title)
    chunk = struct.pack(">H", title_length)
    chunk += title.encode("utf-16be")

    mode = obj["data"]["mode"].encode()
    values = obj["data"]["values"]
    color_type = obj["type"]

    fmt = {b"RGB": "!fff", b"Gray": "!f", b"CMYK": "!ffff", b"LAB": "!fff"}
    if mode in fmt:
        padded_mode = mode.decode().ljust(4).encode()
        chunk += struct.pack("!4s", padded_mode)  # the color mode
        chunk += struct.pack(fmt[mode], *values)  # the color values

    color_types = ["Global", "Spot", "Process"]
    if color_type in color_types:
        color_int = color_types.index(color_type)
        chunk += struct.pack(">h", color_int)  # append swatch mode

    chunk = struct.pack(">I", len(chunk)) + chunk  # prepend the chunk size
    return b"\x00\x01" + chunk  # swatch color header
    # }


def _chunk_for_folder(obj):
    # {  ###
    """produce a byte-chunk for a folder of colors

    the structure is very similar to a color's data:
    • Header
        b'\xc0\x01' +
        Big Endian Unsigned Int == len(Bytes in the Header Block)
          note _only_ the header, this doesn't include the length of color data
          • Big Endian Unsigned Short == len(Folder Name + '\0')
              Note that Folder Name is assumed to be utf-16be so this
              will always be an even number
          • Folder Name + '\0', encoded UTF-16BE
    • body
        chunks for each color, see _chunk_for_color
    • folder terminator
        b'\xc0\x02' +
        b'\x00\x00\x00\x00'

    Perhaps the four null bytes represent something, but i'm pretty sure
    they're just a terminating string, but there's something nice about
    how the b'\xc0\x02' matches with the folder's header
    """
    title = obj["name"] + "\0"
    title_length = len(title)
    chunk_body = struct.pack(">H", title_length)  # title length
    chunk_body += title.encode("utf-16be")  # title

    chunk_head = b"\xc0\x01"  # folder header
    chunk_head += struct.pack(">I", len(chunk_body))
    # precede entire chunk by folder header and size of folder
    chunk = chunk_head + chunk_body

    chunk += b"".join([_chunk_for_color(c) for c in obj["swatches"]])

    chunk += b"\xc0\x02"  # folder terminator chunk
    chunk += b"\x00\x00\x00\x00"  # folder terminator
    return chunk
    # }


# ACO.
# ============================================================
@unique
class ColorSpace(Enum):
    # {  ###
    """Adobe Color Swatch - Color Space Ids."""

    RGB = (0, "RGB", True)
    HSB = (1, "HSB", True)
    CMYK = (2, "CMYK", True)
    PANTONE = (3, "Pantone matching system", False)
    FOCOLTONE = (4, "Focoltone colour system", False)
    TRUMATCH = (5, "Trumatch color", False)
    TOYO = (6, "Toyo 88 colorfinder 1050", False)
    LAB = (7, "Lab", False)
    GRAYSCALE = (8, "Grayscale", True)
    HKS = (10, "HKS colors", False)

    def __new__(cls, *args):
        obj = object.__new__(cls)
        obj._value_ = args[0]
        return obj

    def __init__(
        self,
        _: int,
        label: str | None = None,
        supported: bool = False,
    ):
        self.label = label
        self.supported = supported

    def __str__(self) -> str:
        return self.label if self.label is not None else "unknown"

    # }


class HexColor(NamedTuple):
    # {  ###
    name: str
    color_space: ColorSpace
    color_hex: str
    # }


class RawColor(NamedTuple):
    # {  ###
    name: str
    color_space: ColorSpace
    component_1: int = 0
    component_2: int = 0
    component_3: int = 0
    component_4: int = 0
    # }


def save_aco_file(colors_data: list[RawColor], file: BinaryIO) -> None:
    # {  ###
    """Saves provided color data into a `.aco` file.

    Args:
        colors_data: list of `RawColor`s, were each of them contains the name,
            color space and four color components.
        file: handle to the `.aco` file to be saved.
    """
    try:
        # Version 1
        version = 1
        file.write(version.to_bytes(2, "big"))

        color_count = len(colors_data)
        file.write(color_count.to_bytes(2, "big"))

        for color_data in colors_data:
            color_space_id = color_data.color_space.value
            file.write(color_space_id.to_bytes(2, "big"))

            file.write(color_data.component_1.to_bytes(2, "big"))
            file.write(color_data.component_2.to_bytes(2, "big"))
            file.write(color_data.component_3.to_bytes(2, "big"))
            file.write(color_data.component_4.to_bytes(2, "big"))

        # Version 2
        version = 2
        file.write(version.to_bytes(2, "big"))

        color_count = len(colors_data)
        file.write(color_count.to_bytes(2, "big"))

        for color_data in colors_data:
            color_space_id = color_data.color_space.value
            file.write(color_space_id.to_bytes(2, "big"))

            file.write(color_data.component_1.to_bytes(2, "big"))
            file.write(color_data.component_2.to_bytes(2, "big"))
            file.write(color_data.component_3.to_bytes(2, "big"))
            file.write(color_data.component_4.to_bytes(2, "big"))

            # + 1 is for termination character
            color_name_length = len(color_data.name) + 1
            file.write(color_name_length.to_bytes(4, "big"))

            color_name_bytes = bytes(color_data.name, "utf-16-be")
            file.write(color_name_bytes)

            termination_char = 0
            file.write(termination_char.to_bytes(2, "big"))

    finally:
        file.close()
    # }


###
