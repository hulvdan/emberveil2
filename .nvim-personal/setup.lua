local opts = { remap = false, silent = true }

vim.keymap.set("n", "<leader>fc", ":e codegen/hands/bf_codegen.cpp<CR>", opts)
vim.keymap.set("n", "<leader>fl", ":e src/bf_lib.cpp<CR>", opts)
vim.keymap.set("n", "<C-S-g>v", function()
    vim.fn.system([[start .cmake/vs17/game.sln]])
end, opts)
vim.keymap.set("n", "<C-S-g>r", function()
    vim.fn.system(
        string.format(
            [[set REAPER_LAUNCH_CWD=%s && start "c:/Program Files/REAPER (x64)/reaper.exe" assets/sfx/src/sfx.rpp]],
            vim.fn.getcwd()
        )
    )
end, opts)

local danger = false
vim.keymap.set("n", "<leader>9", function()
    if danger then
        vim.fn.execute("hi normal guibg=#300000")
    else
        vim.fn.execute("hi normal guibg=clear")
    end
    danger = not danger
end, opts)

function cli_command(cmd)
    return [[uvx ruff check --output-format concise cli && uv run cli\bf_cli.py ]] .. cmd
end

target = "game"
platform = "Win"
build_type = "Debug"

function select_target()
    targets = { "game" }
    build_types = { "Debug", "Release", "RelWithDebInfo" }
    platforms = { "Win", "Web", "WebYandex", "WebItch" }

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
        { "d_run_in_debugger", cli_command(string.format("run_in_debugger %s Debug", target)) },
        { "f_run_in_debugger_tests", cli_command("run_in_debugger tests Debug") },
        { "u_update_template", cli_command("update_template") },
        { "t_test", cli_command("test") },
        {
            "y_test_python",
            [[uvx ruff check --output-format concise cli && uv run pytest -x -vv]],
        },
        { "r_build_all_and_test", cli_command("build_all_and_test") },
        {
            "z_serve_web_debug",
            function()
                vim.fn.execute([[term python -m http.server -d .cmake\Web_Debug -b 0.0.0.0 8000]])
            end,
        },
        {
            "x_serve_web_release",
            function()
                vim.fn.execute([[term python -m http.server -d .cmake\Web_Release -b 0.0.0.0 8001]])
            end,
        },
        {
            "c_serve_webyandex_release",
            function()
                vim.fn.execute(
                    [[term npx @yandex-games/sdk-dev-proxy --dev-mode=true -c -p .cmake\WebYandex_Release --port 8082]]
                )
            end,
        },
        { "o_deploy_itch", cli_command("deploy_itch") },
        { "p_deploy_yandex", cli_command("deploy_yandex") },
        { "i_make_swatch", cli_command("make_swatch") },
        { "l_process_images", cli_command("process_images") },
        { "g_codegen", cli_command("codegen Win Debug") },
        -- -- { "killall", [[start .nvim-personal\cli.ahk killall]] },
        { "l_lint_cpp", cli_command("lint") },
        {
            "b_banner",
            function()
                vim.fn.execute([[term uv run python cli\bf_cli.py banner ]] .. vim.fn.expand("%"))
            end,
        },
        { "h_shaders", cli_command("shaders") },
        {
            "v_receive_ws_logs",
            function()
                vim.fn.execute([[term uv run python cli\bf_cli.py receive_ws_logs 8003]])
            end,
        },
        { "w_temp", cli_command("temp") },
        -- { "list_sounds", cli_command("list_sounds") },
        -- { "z_clean_cmake", [[del /f/s/q .cmake]] },
        -- { "x_clean_temp", [[del /f/s/q .temp]] },
        -- ----------
        -- { "boner_build_debug", cli_command("build_boner_debug") },
        -- { "boner_run_in_debugger_debug", cli_command("boner_run_in_debugger_debug") },
        -- { "crop_video", cli_command("crop_video") },
        -- { "export_gif", cli_command("export_gif") },
    })
end

rebuild_tasks()

-- Insert ZoneScopedN below comment line.
vim.keymap.set(
    "n",
    "<leader>z",
    '^wy$o<BS><BS><BS>ZoneScopedN("<ESC>pa");<ESC>VJ>o<ESC>',
    { remap = true, silent = true }
)

vim.keymap.set("n", "<F4>", "<leader>aa", { remap = true, silent = true })
vim.keymap.set("n", "<F5>", "<leader>ae", { remap = true, silent = true })
vim.keymap.set("n", "<F6>", "<leader>ad", { remap = true, silent = true })
vim.keymap.set("n", "<F7>", "<leader>af", { remap = true, silent = true })

-- Space + M -> настройки игры пользователя.
vim.keymap.set("n", "<leader>m", function()
    vim.fn.execute("e C:/Users/user/AppData/Roaming/HulvdanTheGame/user_settings.variables")
end, opts)

------------------------------------------------------------------------------------
-- Остальное.
------------------------------------------------------------------------------------

-- Errorformat.
vim.fn.execute([[set errorformat=]])
-- Python.
vim.fn.execute([[set errorformat+=%f:%l:%c:\ %m]])
-- Pyright
vim.fn.execute([[set errorformat+=\ \ %f:%l:%c\ -\ %t%[A-z]%#:\ %m]])
-- Web.
vim.fn.execute([[set errorformat+=%f(%l\\,%c):\ %t%[A-z]%#\ %m]])
vim.fn.execute([[set errorformat+=%f:%l:%c:\ %t%[A-z]%#:\ %m]])
-- MSBuild.
-- https://forums.handmadehero.org/index.php/forum?view=topic&catid=4&id=704#3982
vim.fn.execute([[set errorformat+=\\\ %#%f(%l)\ :\ %#%t%[A-z]%#\ %m]])
vim.fn.execute([[set errorformat+=\\\ %#%f(%l\\\,%c-%*[0-9]):\ %#%t%[A-z]%#\ %m]])
-- FlatBuffers.
vim.fn.execute([[set errorformat+=\ \ %f(%l\\,\ %c\\):\ %m]])
-- Not sure what these are for.
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
