# Hierarchical Finite State Machines

## VSCode Snippets

Go to `File | Preferences | Configure User settings`.
Select `c.json`

Add the following:

```json
{
 "hfsm state": {
  "prefix": ["state", "hfsm-state"],
  "body": [
   "static az_result $1(az_event_policy* me, az_event event)",
   "{",
   "  az_result ret = AZ_OK;",
   "  (void)me;",
   "",
   "  if (_az_LOG_SHOULD_WRITE(event.type))",
   "  {",
   "    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR(\"$2/$1\"));",
   "  }",
   "",
   "  switch (event.type)",
   "  {",
   "    case AZ_HFSM_EVENT_ENTRY:",
   "      // TODO $3",
   "      break;",
   "",
   "    case AZ_HFSM_EVENT_EXIT:",
   "      // TODO $4",
   "      break;",
            "",
   "    default:",
   "      // TODO $5",
   "      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;",
   "      break;",
   "  }",
   "",
   "  return ret;",
   "}"
  ],
  "description": "Azure Embedded C SDK HFSM State"
 },
 "hfsm parent": {
  "prefix": ["parent", "get-parent", "hfsm-get-parent"],
  "body": [
   "static az_result root(az_event_policy* me, az_event event);",
   "static az_result idle(az_event_policy* me, az_event event);",
   "",
   "static az_event_policy_handler _get_parent(az_event_policy_handler child_state)",
   "{",
   "  az_event_policy_handler parent_state;",
   "",
   "  if (child_state == root)",
   "  {",
   "    parent_state = NULL;",
   "  }",
   "  else if (child_state == idle)",
   "  {",
   "    parent_state = root;",
   "  }",
   "  else",
   "  {",
   "    // Unknown state.",
   "    az_platform_critical_error();",
   "    parent_state = NULL;",
   "  }",
   "",
   "  return parent_state;",
   "}"
  ],
  "description": "Azure Embedded C SDK HFSM hierachy definition (_get_parent)"
 }
}
```

