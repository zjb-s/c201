g = grid.connect()
repeats = {
    1,1,1,1,1,1,1,1
}
trig_modes = {
    1,1,1,1,1,1,1,1
}

function init() 

end

function grid_redraw()
    g:all(4)
    for k,v in ipairs(repeats) do
        for int i=0,3 do
            g:led(k,i, 15);
        end
    end
    g:refresh()
end