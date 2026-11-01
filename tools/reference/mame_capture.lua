-- MAME reference capture for Guardians (development only).
-- Writes PC samples, work-RAM dumps at selected frames and PNG snapshots.
local frame = 0
local log = io.open("build/reference/mame_frames.txt", "w")
local ram_frames = { [300] = true, [600] = true, [1200] = true, [2400] = true, [4800] = true }
local snap_frames = { [300] = true, [600] = true, [1200] = true, [2400] = true }

local function on_frame()
  frame = frame + 1
  local m = manager.machine
  local cpu = m.devices[":maincpu"]
  local sp = cpu.spaces["program"]
  local pc = cpu.state["PC"].value
  local sr = cpu.state["SR"].value
  log:write(string.format("%d %06x %04x\n", frame, pc, sr))
  if frame % 60 == 0 then log:flush() end

  if ram_frames[frame] then
    local f = io.open(string.format("build/reference/ram_%d.bin", frame), "wb")
    local chunk = {}
    for a = 0, 0xFFFF, 2 do
      chunk[#chunk + 1] = string.char(sp:read_u16(0x200000 + a) >> 8,
                                      sp:read_u16(0x200000 + a) & 0xFF)
    end
    f:write(table.concat(chunk))
    f:close()
    print(string.format("RAM dumped at frame %d", frame))
  end

  if snap_frames[frame] then
    local screen = m.screens[":screen"]
    local path = string.format("build/reference/snap/frame_%04d.png", frame)
    local err = screen:snapshot(path)
    print(string.format("snapshot %d: %s", frame, tostring(err)))
  end
end

emu.register_frame_done(on_frame, "frame")
