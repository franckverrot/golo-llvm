define i32 @main(i32 %argc, i8** %argv) {
entry:
  call i64 @_golo_entry_point(i64 undef)
  ret i32 0
}
