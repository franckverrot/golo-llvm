module llvm_golo
import crt

function wow  = |such| {
  return such * 3
}

function very = |omg, amazing| {
  return omg * amazing
}

function main = |args| {
  #let makeSomeNoise = "BOOM!!1!"
  let seven = 7
  echo(wow(very(2,seven)))
  return 0
}
