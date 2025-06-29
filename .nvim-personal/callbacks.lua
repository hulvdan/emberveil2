function table_contains(tbl, x)
    found = false
    for _, v in pairs(tbl) do
        if v == x then
            found = true
        end
    end
    return found
end

bufenter_function = function()
    local extension = vim.fn.expand("%"):match("^.+(%..+)$") -- example: `.log`

    if extension == ".fbs" then
        vim.fn.execute("setlocal commentstring=//%s")
        return
    end

    if table_contains({ ".vs", ".fs" }, extension) then
        vim.fn.execute("set filetype=glsl")
        return
    end

    if table_contains({ ".c", ".h", ".cpp", ".hpp" }, extension) then
        vim.fn.execute("set shiftwidth=2")
    end
end

vim.g.hulvdan_bufenter_callbacks_function = bufenter_function

-- Делаем так, чтобы callback был установлен лишь один раз.
if not vim.g.hulvdan_callbacks_were_set then
    vim.g.hulvdan_callbacks_were_set = true

    vim.api.nvim_create_autocmd({ "BufEnter" }, {
        pattern = "*",
        once = false,
        callback = function()
            if vim.g.hulvdan_bufenter_callbacks_function ~= nil then
                vim.g.hulvdan_bufenter_callbacks_function()
            end
        end,
    })
end
