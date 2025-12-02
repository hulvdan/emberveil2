local opts = { remap = false, silent = true }

vim.keymap.set("n", "gD", "<C-w>o:vs<CR>gd", opts)
vim.keymap.set("n", "<leader>fc", ":e codegen/hands/bf_codegen.cpp<CR>", opts)

vim.keymap.set("n", "<leader>C", "o  continue;<ESC>", opts)
vim.keymap.set("n", "<leader>B", "o  break;<ESC>", opts)

function cli_command(cmd)
    return [[uvx ruff check --output-format concise cli && uv run mypy --check-untyped-defs cli && uv run cli\bf_cli.py ]]
        .. cmd
end

target = "game"
platform = "Win"
build_type = "Debug"

function select_target()
    targets = { "game" }
    build_types = { "Debug", "Release", "RelWithDebInfo" }
    platforms = { "Win", "Web", "WebYandex" }

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
            [[uvx ruff check --output-format concise cli && uv run mypy --check-untyped-defs cli && uv run pytest -x -vv]],
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
                vim.fn.execute([[term npx @yandex-games/sdk-dev-proxy -h localhost --dev-mode=true]])
                vim.fn.execute([[term python -m http.server -d .cmake\WebYandex_Release 80]])
            end,
        },
        { "o_deploy_itch", cli_command("deploy_itch") },
        { "p_deploy_yandex", cli_command("deploy_yandex") },
        { "i_make_swatch", cli_command("make_swatch") },
        { "g_codegen", cli_command("codegen Win Debug") },
        -- -- { "killall", [[start .nvim-personal\cli.ahk killall]] },
        { "l_lint_cpp", cli_command("lint") },
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
