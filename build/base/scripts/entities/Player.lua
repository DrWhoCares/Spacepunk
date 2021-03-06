-- Main player actor script!

function animate(name, blend)
	model:animate(name, blend)
	gun:animate(name, blend)
	animation = name;
end

function init()
	model = entity:findModelByName("model")
	gun = entity:findModelByName("gun")
	bbox = entity:findBBoxByName("physics")
	animate("idle", false)
end

function process()
	if (entity:isMoving() and animation ~= "walk forward") then
		animate("walk forward", true)
	elseif (not entity:isMoving() and animation ~= "idle") then
		animate("idle", true)
	end
end

function postprocess()
end

function preprocess()
end