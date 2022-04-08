function init()
    output[2].action = pulse(0.05, 10, 1)
    output[4].action = pulse(0.05, 5, 1)
end

function playnote(n, v)
    output[1].volts = n / 12
    output[2]()
end