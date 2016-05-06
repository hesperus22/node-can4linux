{
  "targets": [
    {
      "target_name": "can4linux",
      "sources": [
        "can4linux.cc",
      ],
      "include_dirs": ["<!(node -e \"require('nan')\")"]
    }
  ]
}
