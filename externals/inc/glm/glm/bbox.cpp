#pragma once
#include "bbox.h"

const glm::bbox glm::bbox::Zero = glm::bbox(glm::vec3::Zero, glm::vec3::Zero, glm::vec3::Zero);
const glm::bbox glm::bbox::One = glm::bbox(glm::vec3::Zero, glm::vec3::One, glm::vec3::One*0.5f);
const glm::bbox glm::bbox::Identity = glm::bbox(-glm::vec3::One * 0.5f, glm::vec3::One * 0.5f, glm::vec3::Zero);
const glm::bbox glm::bbox::Infinity = glm::bbox(glm::vec3::Infinity, -glm::vec3::Infinity, glm::vec3::Infinity);
