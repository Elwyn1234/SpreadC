char stringEquals(String a, String b) {
  if (a.length != b.length) {
    return FALSE;
  }

  for (int i = 0; i < a.length; i++) {
    if (a.value[i] != b.value[i]) {
      return FALSE;
    }
  }

  return TRUE;
}

