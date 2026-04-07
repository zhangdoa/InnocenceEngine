import { reactive } from 'vue'

export const editorState = reactive({
  entities: [],
  selectedEntity: null,
  selectedEntityId: null,
  sharedHandle: null,
  isConnected: false,
  lastMessage: ''
})
