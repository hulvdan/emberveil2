vim.keymap.set("n", "gD", "<C-w>o:vs<CR>gd", opts)

local opts = { remap = false, silent = true }

function cli_command(cmd)
    return [[.venv\Scripts\ruff.exe check cli && .venv\Scripts\mypy.exe cli && .venv\Scripts\python.exe cli\cli.py ]]
        .. cmd
end

target = "game"
platform = "Win"
build_type = "Debug"

function select_target()
    targets = { "game" }
    build_types = { "Debug", "Release", "RelWithDebInfo" }
    platforms = { "Win", "Web" }

    function platform_build_type_choose()
        require("fastaction").select(platforms, {}, function(selected_platform)
            platform = selected_platform

            require("fastaction").select(build_types, {}, function(selected_build_type)
                build_type = selected_build_type

                print("Active configuration:", target, platform, build_type)
                rebuild_tasks()
            end)
        end)
    end

    if #targets > 1 then
        require("fastaction").select(targets, {}, function(selected_target)
            target = selected_target
            platform_build_type_choose()
        end)
    else
        platform_build_type_choose()
    end
end

function rebuild_tasks()
    -- Space + A -> Куча команд.
    vim.g.hulvdan_tasks({
        { "a_select_target", select_target },
        { "e_build", cli_command(string.format("build %s %s %s", target, platform, build_type)) },
        { "d_debug", cli_command(string.format("run_in_debugger %s Debug", target)) },
        { "u_update_template", cli_command("update_template") },
        -- { "t_test", cli_command("test") },
        -- { "p_test_python", [[.venv\Scripts\pytest.exe]] },
        -- -- { "killall", [[start .nvim-personal\cli.ahk killall]] },
        -- { "l_lint_cpp", cli_command("lint") },
        -- { "k_lint_python", [[.venv\Scripts\ruff.exe check cli]] },
        -- { "z_clean_cmake", [[del /f/s/q .cmake]] },
        -- { "x_clean_temp", [[del /f/s/q .temp]] },
        -- { "s_shaders_build_debug", cli_command("build_shaders_debug") },
        -- { "r_shaders_run_in_debugger_debug", cli_command("shaders_run_in_debugger_debug") },
        -- ----------
        -- { "o_deploy_itch", cli_command("deploy_itch") },
        -- { "boner_build_debug", cli_command("build_boner_debug") },
        -- { "boner_run_in_debugger_debug", cli_command("boner_run_in_debugger_debug") },
        -- { "crop_video", cli_command("crop_video") },
        -- { "export_gif", cli_command("export_gif") },
    })
end

rebuild_tasks()

vim.keymap.set("n", "<F4>", "<leader>aa", { remap = true, silent = true })
vim.keymap.set("n", "<F5>", "<leader>ae", { remap = true, silent = true })
vim.keymap.set("n", "<F6>", "<leader>ad", { remap = true, silent = true })

-- Space + M -> настройки игры пользователя.
vim.keymap.set("n", "<leader>m", function()
    vim.fn.execute("e C:/Users/user/AppData/Roaming/HulvdanTheGame/user_settings.variables")
end, opts)

-- Space + 0 -> Folding of `///`.
vim.keymap.set("n", "<leader>0", function()
    if (vim.bo.filetype == "cpp") or (vim.bo.filetype == "jsonc") or (vim.bo.filetype == "yaml") then
        vim.fn.execute([[silent!normal!zE]])
        vim.fn.execute([[silent!normal!mz]])
        vim.fn.execute([[%g/\/\/\//silent!normal! j$zf%]])
        vim.fn.execute([[%g/###/silent!normal! j$zf%]])
        vim.api.nvim_input("hl0$`z")
    end
end, opts)

------------------------------------------------------------------------------------
-- Остальное.
------------------------------------------------------------------------------------

vim.fn.execute([[set errorformat=]])

-- Обработка ошибок web.
vim.fn.execute([[set errorformat+=%f(%l\\,%c):\ %t%[A-z]%#\ %m]])
vim.fn.execute([[set errorformat+=%f:%l:%c:\ %t%[A-z]%#:\ %m]])

-- Обработка ошибок MSBuild.
-- https://forums.handmadehero.org/index.php/forum?view=topic&catid=4&id=704#3982
-- Microsoft MSBuild
-- Microsoft compiler: cl.exe
vim.fn.execute([[set errorformat+=\\\ %#%f(%l)\ :\ %#%t%[A-z]%#\ %m]])
-- Microsoft HLSL compiler: fxc.exe
vim.fn.execute([[set errorformat+=\\\ %#%f(%l\\\,%c-%*[0-9]):\ %#%t%[A-z]%#\ %m]])

-- Обработка ошибок FlatBuffers.
vim.fn.execute([[set errorformat+=\ \ %f(%l\\,\ %c\\):\ %m]])

vim.fn.execute([[set errorformat+=%f:%l:\ %m]])

-- Форматтер.
vim.g.hulvdan_conform_exclude_formatting_patterns =
    { [[^%.venv/]], [[^vendor/]], [[^%.venv\]], [[^vendor\]], [[^codegen\]], [[^codegen]] }

local function first(bufnr, ...)
    local conform = require("conform")
    for i = 1, select("#", ...) do
        local formatter = select(i, ...)
        if conform.get_formatter_info(formatter, bufnr).available then
            return formatter
        end
    end
    return select(1, ...)
end

require("conform").setup({
    formatters = {
        cog = {
            command = [[.venv\Scripts\cog.exe]],
            args = { "-r", "$FILENAME" },
            stdin = false,
        },
    },
    formatters_by_ft = {
        cpp = function(bufnr)
            return { "cog", "good_clang_format" }
        end,
        jsonc = function(bufnr)
            return { first(bufnr, "prettierd", "prettier") }
        end,
        yaml = function(bufnr)
            return { "yamlfmt" }
        end,
        markdown = function(bufnr)
            return { "cog" }
        end,
    },
})
