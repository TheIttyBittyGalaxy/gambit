-- This is a lua script designed to incrementally compile the compiler.
-- The script depends on the Windows command prompt.
-- g++ must be available via the path.
--
-- gcc version 8.2.0
-- lua version 5.4.2

-- FLAGS --
local REBUILD_ALL = false

local arg_errors = false
for _, flag in ipairs(arg) do
    if flag == "-r" or flag == "-rebuild" then
        REBUILD_ALL = true
    else
        arg_errors = true
    end
end

if arg_errors then
    error("USAGE: do build [-rebuild]")
end

-- CURRENT TIME --
-- When the dir command lists the last write time of each file, it does not specify the second.
-- For this reason, it is possible to edit, build, then re-edit a source file all in one minute,
-- and the script would not be able to tell. Having already cached the current minute, the script
-- would not rebuild the file after the second edit.
--
-- For this reason, if a cached file was last built during the same minute as the current time, we
-- rebuilt it anyway.

local CURRENT_TIME = tonumber(os.date("%Y%m%d%H%M"))

-- GLOBALS --

local total_time = 0
local skipped_files = false

-- PATTERNS --
local CACHE_PATTERN = "(.+)\t(.+)"
local DIR_PATTERN = "(%d%d%d%d)%-(%d%d)%-(%d%d)%s+(%d%d):(%d%d) (%u%u)%s+(%d+)%s+(.+)"

-- READ CACHE --
local cache = {}
local cache_file = io.open("local/build/.build-cache", "r")
if cache_file then
    for line in cache_file:lines() do
        local time, name = line:match(CACHE_PATTERN)
        if name then
            cache[name] = tonumber(time)
        end
    end
    cache_file:close()
else
    print("WARNING: Could not open build cache. This will prevent incremental builds.")
end

-- READ HEADER FILES --
local header_dir_handle = io.popen("dir compiler\\*.h /t:w /-c /o:-d")
if not header_dir_handle then
    error("ERROR: Could not find compiler .h files")
end

-- CHECK HEADER FILES FOR REBUILD --
for line in header_dir_handle:lines() do

    -- Parse name and time from line
    local year, month, day, hour, min, meridiem, _, name = line:match(DIR_PATTERN)

    -- Check that line from dir handle actually represents a file
    if name then

        -- Determine last time written
        local time_last_written = tonumber(year .. month .. day .. hour .. min)
        if meridiem == "PM" then
            time_last_written = time_last_written + 1200
        end

        -- Check if the compiler needs to be rebuilt
        local cached_time = cache[name]
        if REBUILD_ALL or not cached_time or cached_time == CURRENT_TIME or cached_time < time_last_written then
            if not REBUILD_ALL then
                print("Rebuilding entire project as header files have been modified")
                print()
            end
            REBUILD_ALL = true
            cache[name] = time_last_written
        end
    end
end

-- READ SOURCE FILES --
local source_dir_handle = io.popen("dir compiler\\*.cpp /t:w /-c /o:-d")
if not source_dir_handle then
    error("ERROR: Could not find compiler .cpp files")
end

-- BUILD SOURCE FILES --
local failed_builds = false
for line in source_dir_handle:lines() do

    -- Parse name and time from line
    local year, month, day, hour, min, meridiem, _, name = line:match(DIR_PATTERN)

    -- Check that line from dir handle actually represents a file
    if name then

        -- Determine last time written
        local time_last_written = tonumber(year .. month .. day .. hour .. min)
        if meridiem == "PM" then
            time_last_written = time_last_written + 1200
        end

        -- Build file
        local cached_time = cache[name]
        if REBUILD_ALL or not cached_time or cached_time == CURRENT_TIME or cached_time < time_last_written then
            print("> " .. name)

            local cmd = ("g++ -g --std=c++17 -c -o local/build/%s.o compiler/%s "):format(name:sub(1, -5), name)
            local start_time = os.clock()
            local success = os.execute(cmd)
            local time_taken = os.clock() - start_time

            if success then
                print(("  %.2f seconds"):format(time_taken))
                total_time = total_time + time_taken
                cache[name] = time_last_written
            else
                print("  Build failed")
                failed_builds = true
            end
        else
            skipped_files = true
        end
    end
end

-- SAVE CACHE --
cache_file = io.open("local/build/.build-cache", "w")
if cache_file then
    for name, time in pairs(cache) do
        cache_file:write(time .. "\t" .. name .. "\n")
    end
else
    print("WARNING: Could not save build cache. This will prevent incremental builds.")
end

-- FINAL BUILD --
if failed_builds then
    error("ERROR: Not all source files could be compiled")
end

print("> Final build")
local start_time = os.clock()
local final_build_success = os.execute("g++ -g --std=c++17 -lm -o local/build/main.exe local/build/*.o")
local time_taken = os.clock() - start_time

if final_build_success then
    total_time = total_time + time_taken
    print(("  %.2f seconds"):format(time_taken))
    if not skipped_files then
        print(("  (%.2f seconds in total)"):format(total_time))
    end
else
    error("ERROR: Error while linking object files in final build")
end
