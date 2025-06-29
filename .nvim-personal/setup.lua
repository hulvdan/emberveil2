vim.keymap.set("n", "gD", "<C-w>o:vs<CR>gd", opts)

local opts = { remap = false, silent = true }

function cli_command(cmd)
    return [[.venv\Scripts\ruff.exe check cli && .venv\Scripts\python.exe cli\cli.py ]] .. cmd
end

-- Space + A -> Куча команд.
vim.g.hulvdan_tasks({
    -- { "d_recording_drop", [[rm .cmake/vs17/Debug/latest_recording.bin]] },
    -- {
    --     "w_recording_setup",
    --     [[cp .cmake/vs17/Debug/latest_recording_enriched.bin .cmake/vs17/Debug/latest_recording.bin]],
    -- },
    { "a_game_build_debug", cli_command("build_game_debug") },
    -- { "n_game_build_release", cli_command("build_game_release") },
    { "b_build_game_web", cli_command("build_game_web") },
    { "e_game_run_in_debugger_debug", cli_command("game_run_in_debugger_debug") },
    -- { "t_game_run_in_debugger_release", cli_command("game_run_in_debugger_release") },
    -- { "l_ldtk_decorations", cli_command("ldtk_decorations") },
    -- { "c_cog", cli_command("cog") },
    -- { "g_generate", cli_command("generate") },
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

vim.keymap.set("n", "<F5>", "<leader>ae", { remap = true, silent = true })

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
        black = { command = [[.venv\Scripts\black.exe]] },
        isort = { command = [[.venv\Scripts\isort.exe]] },
        cog = {
            command = [[.venv\Scripts\cog.exe]],
            args = { "-r", "$FILENAME" },
            stdin = false,
        },
        shader = {
            command = [[.venv\Scripts\python.exe]],
            args = { "cli/cli.py", "shader", "$FILENAME" },
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
        python = function(bufnr)
            -- return { "cog", "black", "isort" }
            return { "black", "isort" }
        end,
        glsl = function(bufnr)
            return { "cog", "good_clang_format", "shader" }
        end,
    },
})
