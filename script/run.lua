local cmd = "local\\build\\main.exe"
if arg[1] then
    cmd = cmd .. " " .. arg[1]
end

local start_time = os.clock()
os.execute(cmd)
local time_taken = os.clock() - start_time

print()
print(("(%.4f seconds in total)"):format(time_taken))
