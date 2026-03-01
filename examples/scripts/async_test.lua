Async.run(function ()
    local niter = 0
    while Async.wait(5) and niter < 5 do -- Async.wait always returns true
        Core.Debug.message("Hola!")
        niter = niter + 1
    end
end)

-- Async.wait(15) <-- This will throw an error. Only works inside Async tasks created with Async.create
-- Core.Debug.message("Hi")